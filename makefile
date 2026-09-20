.POSIX:

SHELL = /bin/sh

# Project structure variables

SRCDIR  = src
OBJDIR  = obj
BINDIR  = .
MANDIR  = man
TESTDIR = test

# Project artefact variables

OBJS = dynarr.o encoding_bhex.o encoding_b28.o lobi.o str.o cli.o
BIN = lobif

# Compiler variables

CC         = gcc
CPPFLAGS   =
CFLAGS     = -std=c99 -Wall -Wextra -Wpedantic -Wvla -Werror
LDFLAGS    = -lm -lcurl

ifdef DEBUG
	CFLAGS += -Og -g -DDEBUG
else
	CFLAGS += -O3
endif

# Install variables

PREFIX     = /usr/local
DESTBINDIR = $(DESTDIR)$(PREFIX)/bin
DESTMANDIR = $(DESTDIR)$(PREFIX)/share/man/man1
PERMREG    = 644
PERMEXE    = 755

# Phony targets

.PHONY: default clean help test test-encoding

default: $(BINDIR)/$(BIN)

clean:
	rm -rf $(OBJDIR) $(BINDIR)/$(BIN)

help:
	@echo
	@echo " Target     Description"
	@echo " ------     -----------"
	@echo "            Build $(BIN)"
	@echo " install    Install $(BIN)"
	@echo " uninstall  Uninstall $(BIN)"
	@echo " clean      Clean built files"
	@echo " test       Test $(BIN)"
	@echo " $@       Display help"
	@echo

test: test-encoding

test-encoding: $(BINDIR)/$(BIN)
	-$(TESTDIR)/encoding/test.sh $(BINDIR)/$(BIN)

# Compiled file targets

$(BINDIR)/$(BIN): $(OBJS:%=$(OBJDIR)/%)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
