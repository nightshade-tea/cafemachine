MAIN := client
CC := gcc
CFLAGS := -Wall -Wextra -g -I. -I../../include

$(MAIN): $(patsubst %.c,%.o,$(wildcard *.c))

clean:
	rm -rf $(MAIN) *.o

format:
	find . -iname '*.[hc]' | xargs clang-format -i

.PHONY: clean format
