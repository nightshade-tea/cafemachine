PROGRAM := client
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
CFLAGS ?= -Wall -Wextra -g
CFLAGS += -I../../include

$(PROGRAM): $(OBJS)

$(OBJS): $(wildcard *.h) ../../include/tea.h

clean:
	rm -f $(PROGRAM) $(OBJS)

.PHONY: clean
