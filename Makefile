# COPYRIGHT & LICENSE


# -------------------- VARIABLES --------------------
# User-defined variables
SRC_DIR   := src
OBJS_DIR  := objs
INC_DIR   := include
DEPS_DIR  := deps
BUILD_DIR := build

BIN := rawhexdump

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(addprefix $(BUILD_DIR)/,$(subst $(SRC_DIR),$(OBJS_DIR),$(SRCS:.c=.o)))
DEPS := $(addprefix $(BUILD_DIR)/,$(subst $(SRC_DIR),$(DEPS_DIR),$(SRCS:.c=.d)))

# Standard variables
CC      := gcc
CFLAGS  := -std=c89 -I$(INC_DIR) -Wall -Wextra -pedantic -g #-Werror
LDFLAGS := -lc


# -------------------- GOALS --------------------
.PHONY: release clean

# Main goal
release: $(BUILD_DIR)/$(BIN)

# Linking
$(BUILD_DIR)/$(BIN): $(OBJS) $(DEPS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# Compiling
$(BUILD_DIR)/$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

# Dependencies
$(BUILD_DIR)/$(DEPS_DIR)/%.d: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MM -MT $(subst $(SRC_DIR),$(BUILD_DIR)/$(OBJS_DIR),$(<:.c=.o)) -MF $@ $<

# Clean
clean:
	$(RM) -r $(BUILD_DIR)


# -------------------- DEPENDENCIES --------------------
ifneq ($(MAKECMDGOALS), clean)
include $(DEPS)
endif
