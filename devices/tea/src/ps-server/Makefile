MAIN := ps-server
CC := gcc
CFLAGS := -Wall -Wextra -g -I. -I../../include

$(MAIN): $(patsubst %.c,%.o,$(wildcard *.c))

install: $(MAIN)
	cp $(MAIN) /usr/local/bin/
	cp $(MAIN).service /etc/systemd/system/

uninstall:
	rm  /usr/local/bin/$(MAIN)
	rm  /etc/systemd/system/$(MAIN).service

clean:
	rm -rf $(MAIN) *.o

format:
	find . -iname '*.[hc]' | xargs clang-format -i

.PHONY: clean format
