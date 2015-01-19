INSTALL=install
PREFIX=/usr
MANDIR?=/local/man/man1

TARGET := xcape

CFLAGS += -Wall
CFLAGS += `pkg-config --cflags xtst x11`
LDFLAGS += `pkg-config --libs xtst x11`
LDFLAGS += -pthread

all: $(TARGET)

$(TARGET): xcape.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install:
	$(INSTALL) -d -m 0755 $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -d -m 0755 $(DESTDIR)$(PREFIX)$(MANDIR)
	$(INSTALL) -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	$(INSTALL) -m 0644 xcape.1 $(DESTDIR)$(PREFIX)$(MANDIR)/xcape.1

clean:
	rm $(TARGET)

.PHONY: all clean install
