CC?=cc
CFLAGS=-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
all: supervisor
supervisor: supervisor.c
	$(CC) $(CFLAGS) -o $@ $<
clean:
	rm -f supervisor
