# Lab 16 — Profiling
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lm

.PHONY: all clean profile-gprof profile-perf

all: profile_target profile_target_gprof

# Normal build (for perf/valgrind)
profile_target: profile_target.c
	$(CC) $(CFLAGS) -O2 $< -o $@ $(LDFLAGS)

# gprof build
profile_target_gprof: profile_target.c
	$(CC) $(CFLAGS) -pg $< -o $@ $(LDFLAGS)

# Run gprof
profile-gprof: profile_target_gprof
	./profile_target_gprof
	gprof profile_target_gprof gmon.out > gprof_report.txt
	@echo "Report saved to gprof_report.txt"
	@head -50 gprof_report.txt

# Run perf
profile-perf: profile_target
	perf record -g ./profile_target
	perf report --stdio > perf_report.txt
	@echo "Report saved to perf_report.txt"
	@head -50 perf_report.txt

clean:
	rm -f profile_target profile_target_gprof gmon.out \
	      gprof_report.txt perf_report.txt perf.data perf.data.old \
	      cachegrind.out.* callgrind.out.*
