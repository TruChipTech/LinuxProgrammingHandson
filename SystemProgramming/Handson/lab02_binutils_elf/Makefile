# Lab 02 — Binutils & ELF Analysis
# Makefile

CC = gcc
CFLAGS = -g -Wall -Wextra
LDFLAGS = -lm

.PHONY: all clean artifact mystery

all: artifact mystery

artifact: elf_artifact.c
	$(CC) $(CFLAGS) $< -o elf_artifact $(LDFLAGS)
	@echo "Built elf_artifact (with debug symbols)"

mystery: mystery_binary.c
	$(CC) -O2 $< -o mystery $(LDFLAGS)
	strip mystery
	@echo "Built mystery (stripped — for forensics exercise)"

analyze: elf_artifact
	chmod +x binary_analyzer.sh
	./binary_analyzer.sh elf_artifact

clean:
	rm -f elf_artifact mystery *.o analysis_report_*.txt
