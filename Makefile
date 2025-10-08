CFLAGS = -Wall -Wno-format -O3 -flto -ggdb
SRC := $(wildcard *.c)
BIN := $(SRC:.c=)
DOT := $(wildcard *.dot)
PNGS := $(DOT:.dot=.png)
SVGS := $(DOT:.dot=.svg)
JPGS := $(DOT:.dot=.jpg)

.SILENT:

log = @echo "[makefile]:$1"
		
all: $(BIN)
	$(call log, "execute 'make help' to see all options")

%: %.c
	printf "[makefile]: building %15s...\n" "$<"
	@$(CC) $(CFLAGS) -o $@ $<

clean-bin:
	$(call log, "cleaning binaries...")
	@rm -f $(BIN)
	@rm -rf *dSYM
	
clean-dot:
	$(call log, "cleaning .dot files...")
	@rm -f $(DOT)

clean-img:
	$(call log, "cleaning images...")
	@rm -f $(PNGS) $(SVGS) $(JPGS)

clean: clean-bin clean-dot clean-img

run: $(BIN)
	$(call log, "running binaries...")
	@for bin in $(BIN); do ./$$bin; done

check-dot:
	@command -v dot >/dev/null 2>&1 || { echo "[makefile]: 'dot' command not found, install graphviz"; exit 1; }

png: run check-dot
	@for f in *.dot; do \
		[ -f "$$f" ] || continue; \
		printf "[makefile]: generating %15s...\n" "$${f%.dot}.png"; \
		dot "$$f" -Tpng -o "$${f%.dot}.png"; \
	done

svg: run check-dot
	@for f in *.dot; do \
		[ -f "$$f" ] || continue; \
		printf "[makefile]: generating %15s...\n" "$${f%.dot}.svg"; \
		dot "$$f" -Tsvg -o "$${f%.dot}.svg"; \
	done

jpg: run check-dot
	@for f in *.dot; do \
		[ -f "$$f" ] || continue; \
		printf "[makefile]: generating %15s...\n" "$${f%.dot}.jpg"; \
		dot "$$f" -Tjpg -o "$${f%.dot}.jpg"; \
	done

image-prelude:
	$(call log, "generating images from .dot files...")

images: run check-dot image-prelude png svg jpg
	$(call log, "all images generated")
	
everything: run images
	$(call log, "complete")

help:
	$(call log, "Makefile options:")
	$(call log, "  all        - Build all binaries")
	$(call log, "  clean-bin  - Remove all binaries")
	$(call log, "  clean-img  - Remove all images")
	$(call log, "  clean-dot  - Remove all graphviz .dot files")
	$(call log, "  clean      - Remove all binaries and generated files")
	$(call log, "  run        - Execute all binaries")
	$(call log, "  check-dot  - Check if 'dot' command is available")
	$(call log, "  png        - Generate PNG images from .dot files")
	$(call log, "  svg        - Generate SVG images from .dot files")
	$(call log, "  jpg        - Generate JPG images from .dot files")
	$(call log, "  images     - Generate all image formats from .dot files")
	$(call log, "  everything - Run all binaries and generate all images")
	$(call log, "  help       - Display this help message")
