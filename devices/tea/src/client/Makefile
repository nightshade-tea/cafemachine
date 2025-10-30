MAIN_PROGRAMS    := client
CFLAGS           := -Wall -Wextra -g -I../../include

all: format $(MAIN_PROGRAMS)

clean:
	rm -rf $(MAIN_PROGRAMS)

format:
	find $(SRC_DIR) -iname '*.[hc]' | xargs clang-format -i

.PHONY: all clean format
