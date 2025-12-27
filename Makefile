# MIT License
# 
# Copyright (c) 2025 Lorenzo Pegorari (@LorenzoPegorari)
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


# ---------------------------------- VARIABLES ----------------------------------

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

# Standard variables (add "-g -Werror" to CFLAGS for debugging)
CC      := gcc
CFLAGS  := -std=c89 -I$(INC_DIR) -Wall -Wextra -pedantic
LDFLAGS := -lc


# ------------------------------------ GOALS ------------------------------------

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


# -------------------------------- DEPENDENCIES ---------------------------------

ifneq ($(MAKECMDGOALS), clean)
include $(DEPS)
endif
