PROGRAMS := client
CFLAGS   := -Wall -Wextra -g -I../../include

all: format $(PROGRAMS)

clean:
	rm -rf $(PROGRAMS)

format:
	find $(SRC_DIR) -iname '*.[hc]' | xargs clang-format -i

.PHONY: all clean format
