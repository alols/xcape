
TARGET := xcape

CFLAGS += -Wall
CFLAGS += `pkg-config --cflags xtst x11`
LDFLAGS += `pkg-config --libs xtst x11`
LDFLAGS += -pthread

$(TARGET): xcape.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm $(TARGET)

.PHONY: clean
