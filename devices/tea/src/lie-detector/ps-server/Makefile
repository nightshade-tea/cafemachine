PROGRAM := ps-server
CFLAGS ?= -Wall -Wextra -g
LDLIBS += -pthread
PREFIX ?= /usr/local

$(PROGRAM):

install: $(PROGRAM)
	install -D -m 755 $(PROGRAM) $(DESTDIR)$(PREFIX)/bin/$(PROGRAM)
	install -D -m 644 $(PROGRAM).service $(DESTDIR)/etc/systemd/system/$(PROGRAM).service
	systemctl daemon-reload

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(PROGRAM) $(DESTDIR)/etc/systemd/system/$(PROGRAM).service
	systemctl daemon-reload

clean:
	rm -f $(PROGRAM)

.PHONY: install uninstall clean
