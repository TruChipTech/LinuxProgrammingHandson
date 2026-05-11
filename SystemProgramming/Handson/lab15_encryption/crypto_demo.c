/*
 * crypto_demo.c — Encryption Techniques in C using OpenSSL
 *
 * Demonstrates:
 *   1. SHA-256 hashing
 *   2. AES-256-CBC encryption/decryption
 *   3. HMAC-SHA256 generation
 *
 * Build: gcc crypto_demo.c -o crypto_demo -lssl -lcrypto
 *
 * Exercise: Extend this to read files and encrypt/decrypt them.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/err.h>

/* ============================================================
 * Helper: Print hex bytes
 * ============================================================ */
void print_hex(const char *label, const unsigned char *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

/* ============================================================
 * 1. SHA-256 Hashing
 * ============================================================ */
int compute_sha256(const unsigned char *data, size_t len,
                   unsigned char *hash_out) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return -1;

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1 ||
        EVP_DigestUpdate(ctx, data, len) != 1 ||
        EVP_DigestFinal_ex(ctx, hash_out, NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    EVP_MD_CTX_free(ctx);
    return 0;
}

void demo_sha256(void) {
    printf("╔══════════════════════════════════╗\n");
    printf("║  1. SHA-256 Hashing Demo         ║\n");
    printf("╚══════════════════════════════════╝\n");

    const char *messages[] = {
        "Hello, Linux System Programming!",
        "Hello, Linux System Programming!",  /* Same — should produce same hash */
        "Hello, Linux System Programming.",  /* Different — one char changed */
    };

    for (int i = 0; i < 3; i++) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        compute_sha256((unsigned char *)messages[i],
                       strlen(messages[i]), hash);
        printf("  Input:  \"%s\"\n", messages[i]);
        print_hex("  SHA256", hash, SHA256_DIGEST_LENGTH);
        printf("\n");
    }
}

/* ============================================================
 * 2. AES-256-CBC Encryption / Decryption
 * ============================================================ */
int aes_encrypt(const unsigned char *plaintext, int plaintext_len,
                const unsigned char *key, const unsigned char *iv,
                unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    int len, ciphertext_len;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_EncryptUpdate(ctx, ciphertext, &len,
                          plaintext, plaintext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

int aes_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                const unsigned char *key, const unsigned char *iv,
                unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    int len, plaintext_len;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_DecryptUpdate(ctx, plaintext, &len,
                          ciphertext, ciphertext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}

void demo_aes(void) {
    printf("╔══════════════════════════════════╗\n");
    printf("║  2. AES-256-CBC Encrypt/Decrypt  ║\n");
    printf("╚══════════════════════════════════╝\n");

    const char *plaintext = "TOP SECRET: The kernel module password is 'penguin42'";
    printf("  Plaintext:  \"%s\"\n", plaintext);
    printf("  Length:     %zu bytes\n\n", strlen(plaintext));

    /* Generate random key and IV */
    unsigned char key[32];  /* 256-bit key */
    unsigned char iv[16];   /* 128-bit IV */
    RAND_bytes(key, sizeof(key));
    RAND_bytes(iv, sizeof(iv));

    print_hex("  Key", key, sizeof(key));
    print_hex("  IV ", iv, sizeof(iv));

    /* Encrypt */
    unsigned char ciphertext[256];
    int cipher_len = aes_encrypt((unsigned char *)plaintext,
                                  strlen(plaintext),
                                  key, iv, ciphertext);
    if (cipher_len < 0) {
        printf("  ERROR: Encryption failed!\n");
        return;
    }
    printf("\n  Ciphertext (%d bytes):\n  ", cipher_len);
    for (int i = 0; i < cipher_len; i++) {
        printf("%02x", ciphertext[i]);
        if ((i + 1) % 32 == 0) printf("\n  ");
    }
    printf("\n");

    /* Decrypt */
    unsigned char decrypted[256];
    int dec_len = aes_decrypt(ciphertext, cipher_len, key, iv, decrypted);
    if (dec_len < 0) {
        printf("  ERROR: Decryption failed!\n");
        return;
    }
    decrypted[dec_len] = '\0';

    printf("\n  Decrypted:  \"%s\"\n", decrypted);
    printf("  Match:      %s\n\n",
           strcmp(plaintext, (char *)decrypted) == 0 ? "✓ YES" : "✗ NO");
}

/* ============================================================
 * 3. HMAC-SHA256
 * ============================================================ */
void demo_hmac(void) {
    printf("╔══════════════════════════════════╗\n");
    printf("║  3. HMAC-SHA256 Demo             ║\n");
    printf("╚══════════════════════════════════╝\n");

    const char *message = "Transfer $1000 to account 12345";
    const char *secret_key = "my-shared-secret-key";

    unsigned char hmac_result[EVP_MAX_MD_SIZE];
    unsigned int hmac_len = 0;

    HMAC(EVP_sha256(),
         secret_key, strlen(secret_key),
         (unsigned char *)message, strlen(message),
         hmac_result, &hmac_len);

    printf("  Message: \"%s\"\n", message);
    printf("  Key:     \"%s\"\n", secret_key);
    print_hex("  HMAC   ", hmac_result, hmac_len);

    /* Verify: same input → same HMAC */
    unsigned char hmac_verify[EVP_MAX_MD_SIZE];
    unsigned int verify_len = 0;
    HMAC(EVP_sha256(),
         secret_key, strlen(secret_key),
         (unsigned char *)message, strlen(message),
         hmac_verify, &verify_len);

    int match = (hmac_len == verify_len &&
                 CRYPTO_memcmp(hmac_result, hmac_verify, hmac_len) == 0);
    printf("  Verify:  %s\n\n", match ? "✓ VALID" : "✗ INVALID");

    /* Tampered message → different HMAC */
    const char *tampered = "Transfer $9999 to account 12345";
    unsigned char hmac_tampered[EVP_MAX_MD_SIZE];
    unsigned int tampered_len = 0;
    HMAC(EVP_sha256(),
         secret_key, strlen(secret_key),
         (unsigned char *)tampered, strlen(tampered),
         hmac_tampered, &tampered_len);

    int tamper_match = (hmac_len == tampered_len &&
                        CRYPTO_memcmp(hmac_result, hmac_tampered, hmac_len) == 0);
    printf("  Tampered: \"%s\"\n", tampered);
    print_hex("  HMAC   ", hmac_tampered, tampered_len);
    printf("  Verify:   %s (expected: INVALID)\n\n",
           tamper_match ? "✗ VALID (BAD!)" : "✓ INVALID (GOOD!)");
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== OpenSSL Cryptography Demonstration ===\n\n");

    demo_sha256();
    demo_aes();
    demo_hmac();

    /*
     * TODO Exercise: Extend this program to:
     * 1. Accept a filename as command-line argument
     * 2. Read the file contents
     * 3. Compute SHA-256 hash of the file
     * 4. Prompt user for a password
     * 5. Derive a key from the password using PBKDF2:
     *      PKCS5_PBKDF2_HMAC(password, len, salt, salt_len,
     *                         10000, EVP_sha256(), 32, key);
     * 6. Encrypt the file with AES-256-CBC
     * 7. Write salt + IV + ciphertext to a .enc file
     * 8. Decrypt the .enc file and verify against original
     */

    printf("=== Demo Complete ===\n");
    return 0;
}
