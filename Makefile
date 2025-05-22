/* Makefile */

# Target library
lib := libfs.a

# Source files
srcs := disk.c fs.c
objs := $(srcs:.c=.o)
deps := $(objs:.o=.d)

# Compiler and tools
CC       := gcc
AR       := ar
ARFLAGS  := rcs
CFLAGS   := -Wall -Wextra -Werror -MMD -MP -I.

all: $(lib)

$(lib): $(objs)
	@echo "AR   $@"
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	@echo "CC   $@"
	$(CC) $(CFLAGS) -c $< -o $@

-include $(deps)

.PHONY: all clean
clean:
	rm -f $(lib) $(objs) $(deps)
