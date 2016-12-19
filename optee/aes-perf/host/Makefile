# Normally this makefile shouldn't be called directly and we expect the output
# path to be on a certain location to fit together with the other OP-TEE
# gits and helper scripts.
ifeq ($O,)
$(error output path should be specified when calling this makefile)
endif

VERSION = $(shell git describe --always --dirty=-dev 2>/dev/null || echo Unknown)

CC = $(CROSS_COMPILE_HOST)gcc

srcs := aes-perf.c

objs := $(patsubst %.c,$(O)/%.o, $(srcs))

CFLAGS += -Os
# For NAN
CFLAGS += -D_ISOC99_SOURCE=1
# For clock_gettime() etc.
CFLAGS += -D_POSIX_C_SOURCE=199309L
CFLAGS += -DVERSION="$(VERSION)"
CFLAGS += -I. -I../ta -I$(OPTEE_CLIENT_PATH)/out/export/include

LDFLAGS += -L$(OPTEE_CLIENT_PATH)/out/export/lib -lteec -lm

.PHONY: all
all: $(O)/aes-perf

$(O)/aes-perf: $(objs)
	$(echo) '  CC      $@'
	$(q)$(CC) -o $@ $+ $(LDFLAGS)

$(O)/%.o: $(CURDIR)/%.c
	$(q)mkdir -p $(O)/host
	$(echo) '  CC      $@'
	$(q)$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@echo '  CLEAN  $(O)'
	$(q)rm -f $(O)/aes-perf
	$(q)rm -f $(objs)

