
TARGET := xcape

CFLAGS += -Wall
CFLAGS += `pkg-config --cflags xtst`
LDFLAGS += `pkg-config --libs xtst`
LDFLAGS += -pthread

$(TARGET): xcape.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm $(TARGET)

.PHONY: clean
