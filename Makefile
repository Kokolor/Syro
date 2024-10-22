CC = gcc
CFLAGS = -g

BUILD_DIR = build

OUT = $(BUILD_DIR)/syroc

CFILES = $(shell find . -type f -name '*.c')
CFILES_CLEAN = $(patsubst ./%, %, $(CFILES))
OBJECTS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(CFILES_CLEAN))

all: $(OUT)

$(OUT): $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(OUT)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)  # Crée le répertoire de destination si nécessaire
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
