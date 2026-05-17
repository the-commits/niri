SRC_DIR  := src
BIN_DIR  := scripts
SRCS     := $(wildcard $(SRC_DIR)/*.c)
BINS     := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%,$(SRCS))
CMDS     := $(patsubst $(SRC_DIR)/%.c,%,$(SRCS))
CC       := gcc
CFLAGS   := -Wall -Wextra

.PHONY: all clean $(CMDS)

all: $(BINS)

# One phony target per command, e.g. make color-scheme
$(CMDS): %: $(BIN_DIR)/%

$(BIN_DIR)/%: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -f $(BINS)
