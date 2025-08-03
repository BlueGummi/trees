CFLAGS = -Wall -O3 -flto

SRC := $(wildcard *.c)
BIN := $(SRC:.c=)

all: $(BIN)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BIN)

run: $(BIN)
	for bin in $(BIN); do ./$$bin; done

