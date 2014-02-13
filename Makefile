INSTALL=install
PREFIX=/usr
MANDIR?=/local/man/man1

TARGET := xcape

CFLAGS += -Wall
CFLAGS += `pkg-config --cflags xtst x11`
LDFLAGS += `pkg-config --libs xtst x11`
LDFLAGS += -pthread

$(TARGET): xcape.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install:
	$(INSTALL) -Dm 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	$(INSTALL) -Dm 644 xcape.1 $(DESTDIR)$(PREFIX)$(MANDIR)/xcape.1

clean:
	rm $(TARGET)

.PHONY: clean
