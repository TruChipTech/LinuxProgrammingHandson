/*
 * memfs.c — Simple In-Memory FUSE Filesystem
 *
 * Build: gcc -Wall -g memfs.c -o memfs $(pkg-config --cflags --libs fuse3)
 * Mount: mkdir -p /tmp/memfs && ./memfs /tmp/memfs
 * Unmount: fusermount3 -u /tmp/memfs
 */

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define MAX_FILES     64
#define MAX_NAME_LEN  255
#define MAX_DATA_SIZE (1024 * 1024)  /* 1 MB per file */

typedef struct {
    char name[MAX_NAME_LEN + 1];
    int is_dir;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    time_t atime, mtime, ctime;
    char *data;
    size_t size;
    int used;
} memfs_entry_t;

static memfs_entry_t entries[MAX_FILES];

/* Find entry by path */
static memfs_entry_t *find_entry(const char *path) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (entries[i].used && strcmp(entries[i].name, path) == 0) {
            return &entries[i];
        }
    }
    return NULL;
}

/* Find a free slot */
static memfs_entry_t *alloc_entry(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!entries[i].used) {
            memset(&entries[i], 0, sizeof(entries[i]));
            entries[i].used = 1;
            entries[i].atime = entries[i].mtime = entries[i].ctime = time(NULL);
            return &entries[i];
        }
    }
    return NULL;
}

/* FUSE: getattr */
static int memfs_getattr(const char *path, struct stat *st,
                         struct fuse_file_info *fi) {
    (void)fi;
    memset(st, 0, sizeof(*st));

    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        st->st_uid = getuid();
        st->st_gid = getgid();
        return 0;
    }

    memfs_entry_t *e = find_entry(path);
    if (!e) return -ENOENT;

    if (e->is_dir) {
        st->st_mode = S_IFDIR | e->mode;
        st->st_nlink = 2;
    } else {
        st->st_mode = S_IFREG | e->mode;
        st->st_nlink = 1;
        st->st_size = e->size;
    }
    st->st_uid = e->uid;
    st->st_gid = e->gid;
    st->st_atime = e->atime;
    st->st_mtime = e->mtime;
    st->st_ctime = e->ctime;

    return 0;
}

/* FUSE: readdir */
static int memfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags) {
    (void)offset; (void)fi; (void)flags;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    size_t pathlen = strlen(path);
    for (int i = 0; i < MAX_FILES; i++) {
        if (!entries[i].used) continue;

        /* Check if entry is a direct child of path */
        const char *name = entries[i].name;
        if (strcmp(path, "/") == 0) {
            if (name[0] == '/' && strchr(name + 1, '/') == NULL) {
                filler(buf, name + 1, NULL, 0, 0);
            }
        } else {
            if (strncmp(name, path, pathlen) == 0 &&
                name[pathlen] == '/' &&
                strchr(name + pathlen + 1, '/') == NULL) {
                filler(buf, name + pathlen + 1, NULL, 0, 0);
            }
        }
    }
    return 0;
}

/* FUSE: create */
static int memfs_create(const char *path, mode_t mode,
                        struct fuse_file_info *fi) {
    (void)fi;
    if (find_entry(path)) return -EEXIST;

    memfs_entry_t *e = alloc_entry();
    if (!e) return -ENOSPC;

    strncpy(e->name, path, MAX_NAME_LEN);
    e->is_dir = 0;
    e->mode = mode & 0777;
    e->uid = getuid();
    e->gid = getgid();
    e->data = NULL;
    e->size = 0;

    return 0;
}

/* FUSE: open */
static int memfs_open(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    if (!find_entry(path)) return -ENOENT;
    return 0;
}

/* FUSE: read */
static int memfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    (void)fi;
    memfs_entry_t *e = find_entry(path);
    if (!e) return -ENOENT;

    if (offset >= (off_t)e->size) return 0;
    if (offset + size > e->size) size = e->size - offset;

    memcpy(buf, e->data + offset, size);
    e->atime = time(NULL);
    return size;
}

/* FUSE: write */
static int memfs_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi) {
    (void)fi;
    memfs_entry_t *e = find_entry(path);
    if (!e) return -ENOENT;

    size_t new_size = offset + size;
    if (new_size > MAX_DATA_SIZE) return -ENOSPC;

    if (new_size > e->size) {
        e->data = realloc(e->data, new_size);
        if (!e->data) return -ENOMEM;
        /* Zero-fill gap if offset > old size */
        if (offset > (off_t)e->size) {
            memset(e->data + e->size, 0, offset - e->size);
        }
        e->size = new_size;
    }

    memcpy(e->data + offset, buf, size);
    e->mtime = time(NULL);
    return size;
}

/* FUSE: mkdir */
static int memfs_mkdir(const char *path, mode_t mode) {
    if (find_entry(path)) return -EEXIST;

    memfs_entry_t *e = alloc_entry();
    if (!e) return -ENOSPC;

    strncpy(e->name, path, MAX_NAME_LEN);
    e->is_dir = 1;
    e->mode = mode & 0777;
    e->uid = getuid();
    e->gid = getgid();

    return 0;
}

/* FUSE: unlink */
static int memfs_unlink(const char *path) {
    memfs_entry_t *e = find_entry(path);
    if (!e) return -ENOENT;
    if (e->is_dir) return -EISDIR;

    free(e->data);
    e->used = 0;
    return 0;
}

/* FUSE: rmdir */
static int memfs_rmdir(const char *path) {
    memfs_entry_t *e = find_entry(path);
    if (!e) return -ENOENT;
    if (!e->is_dir) return -ENOTDIR;

    /* Check if directory is empty */
    size_t pathlen = strlen(path);
    for (int i = 0; i < MAX_FILES; i++) {
        if (!entries[i].used || &entries[i] == e) continue;
        if (strncmp(entries[i].name, path, pathlen) == 0 &&
            entries[i].name[pathlen] == '/') {
            return -ENOTEMPTY;
        }
    }

    e->used = 0;
    return 0;
}

/* FUSE: truncate */
static int memfs_truncate(const char *path, off_t size,
                          struct fuse_file_info *fi) {
    (void)fi;
    memfs_entry_t *e = find_entry(path);
    if (!e) return -ENOENT;

    if ((size_t)size > MAX_DATA_SIZE) return -ENOSPC;

    e->data = realloc(e->data, size);
    if (size > (off_t)e->size) {
        memset(e->data + e->size, 0, size - e->size);
    }
    e->size = size;
    e->mtime = time(NULL);
    return 0;
}

static const struct fuse_operations memfs_ops = {
    .getattr  = memfs_getattr,
    .readdir  = memfs_readdir,
    .create   = memfs_create,
    .open     = memfs_open,
    .read     = memfs_read,
    .write    = memfs_write,
    .mkdir    = memfs_mkdir,
    .unlink   = memfs_unlink,
    .rmdir    = memfs_rmdir,
    .truncate = memfs_truncate,
};

int main(int argc, char *argv[]) {
    memset(entries, 0, sizeof(entries));
    return fuse_main(argc, argv, &memfs_ops, NULL);
}
