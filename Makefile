CFLAGS = -Wall -O3 -flto
SRC := $(wildcard *.c)
BIN := $(SRC:.c=)
DOT := $(wildcard *.dot)
PNGS := $(DOT:.dot=.png)
SVGS := $(DOT:.dot=.svg)
JPGS := $(DOT:.dot=.jpg)

.SILENT:

all: $(BIN)

%: %.c
	@echo "building $<..."
	@$(CC) $(CFLAGS) -o $@ $<

clean:
	@echo "cleaning..."
	@rm -f $(BIN) $(PNGS) $(SVGS) $(JPGS) $(DOT)

run: $(BIN)
	@echo "running binaries..."
	@for bin in $(BIN); do ./$$bin; done

check-dot:
	@command -v dot >/dev/null 2>&1 || { echo "'dot' command not found, install graphviz"; exit 1; }

png: check-dot $(PNGS)

%.png: %.dot
	@echo "generating $@..."
	@dot $< -Tpng -o $@

svg: check-dot $(SVGS)

%.svg: %.dot
	@echo "generating $@..."
	@dot $< -Tsvg -o $@

jpg: check-dot $(JPGS)

%.jpg: %.dot
	@echo "generating $@..."
	@dot $< -Tjpg -o $@

images: check-dot
	@echo "generating images from .dot files..."
	@for f in *.dot; do \
		[ -f "$$f" ] || continue; \
		dot "$$f" -Tpng -o "$${f%.dot}.png"; \
		dot "$$f" -Tsvg -o "$${f%.dot}.svg"; \
		dot "$$f" -Tjpg -o "$${f%.dot}.jpg"; \
	done

everything: run images
	@echo "everything compiled and all images generated"
