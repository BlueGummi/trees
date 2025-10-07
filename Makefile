CFLAGS = -Wall -Wno-format -O3 -flto -ggdb
SRC := $(wildcard *.c)
BIN := $(SRC:.c=)
DOT := $(wildcard *.dot)
PNGS := $(DOT:.dot=.png)
SVGS := $(DOT:.dot=.svg)
JPGS := $(DOT:.dot=.jpg)

.SILENT:

log = @echo -n "[\e[1;92mmakefile\e[0m]:$1"; echo
ifneq ($(OS), Windows_NT)
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		log = @echo "[makefile]:$1"
	endif
endif
		
all: $(BIN)
	$(call log, "execute 'make help' to see all options")

%: %.c
	$(call log, "building $<...")
	@$(CC) $(CFLAGS) -o $@ $<

clean:
	$(call log, "cleaning...")
	@rm -f $(BIN) $(PNGS) $(SVGS) $(JPGS) $(DOT)
	@rm -rf *dSYM

run: $(BIN)
	$(call log, "running binaries...")
	@for bin in $(BIN); do ./$$bin; done

check-dot:
	@command -v dot >/dev/null 2>&1 || { echo "[\e[1;92mmakefile\e[0m]: 'dot' command not found, install graphviz"; exit 1; }

png: check-dot $(PNGS)

%.png: %.dot
	$(call log, "generating $@...")
	@dot $< -Tpng -o $@

svg: check-dot $(SVGS)

%.svg: %.dot
	$(call log, "generating $@...")
	@dot $< -Tsvg -o $@

jpg: check-dot $(JPGS)

%.jpg: %.dot
	$(call log, "generating $@...")
	@dot $< -Tjpg -o $@

images: check-dot
	$(call log, "generating images from .dot files...")
	@for f in *.dot; do \
		[ -f "$$f" ] || continue; \
		dot "$$f" -Tpng -o "$${f%.dot}.png"; \
		dot "$$f" -Tsvg -o "$${f%.dot}.svg"; \
		dot "$$f" -Tjpg -o "$${f%.dot}.jpg"; \
	done

everything: run images
	$(call log, "everything compiled and all images generated")

help:
	$(call log, "Makefile options:")
	$(call log, "  all        - Build all binaries")
	$(call log, "  clean      - Remove all binaries and generated files")
	$(call log, "  run        - Execute all binaries")
	$(call log, "  check-dot  - Check if 'dot' command is available")
	$(call log, "  png        - Generate PNG images from .dot files")
	$(call log, "  svg        - Generate SVG images from .dot files")
	$(call log, "  jpg        - Generate JPG images from .dot files")
	$(call log, "  images     - Generate all image formats from .dot files")
	$(call log, "  everything - Run all binaries and generate all images")
	$(call log, "  help       - Display this help message")
