# BUILD SETTINGS ###############################################################

ifneq ($(filter Msys Cygwin, $(shell uname -o)), )
    PLATFORM := WIN32
    TYRIAN_DIR = C:\\TYRIAN
else
    PLATFORM := UNIX
    TYRIAN_DIR = $(gamesdir)/tyrian
endif

# true, false, or auto (true if pkg-config can find SDL2_net)
WITH_NETWORK := auto

# true or false (build the C++ extensions in src/tyrx; requires GCC 16+ for C++26 reflection)
WITH_TYRX ?= false

################################################################################

# see https://www.gnu.org/prep/standards/html_node/Makefile-Conventions.html

SHELL = /bin/sh

CC ?= gcc
CXX ?= g++
INSTALL ?= install
NM ?= nm
PYTHON ?= python3
PKG_CONFIG ?= pkg-config
WINDRES ?= windres

VCS_IDREV ?= (git describe --tags || git rev-parse --short HEAD)

INSTALL_PROGRAM ?= $(INSTALL)
INSTALL_DATA ?= $(INSTALL) -m 644

prefix ?= /usr/local
exec_prefix ?= $(prefix)

bindir ?= $(exec_prefix)/bin
datarootdir ?= $(prefix)/share
datadir ?= $(datarootdir)
docdir ?= $(datarootdir)/doc/opentyrian
mandir ?= $(datarootdir)/man
man6dir ?= $(mandir)/man6
man6ext ?= .6
desktopdir ?= $(datarootdir)/applications
icondir ?= $(datarootdir)/icons

# see https://www.pathname.com/fhs/pub/fhs-2.3.html

gamesdir ?= $(datadir)/games

###

TARGET := opentyrian
RES :=
ifeq ($(PLATFORM), WIN32)
    TARGET := opentyrian.exe
    # The icon, from the same resource script the Visual Studio build uses.
    RES := obj/resources.o
endif

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=obj/%.o)
DEPS := $(SRCS:src/%.c=obj/%.d)

ifeq ($(WITH_TYRX), true)
    TYRX_SRCS := $(wildcard src/tyrx/*.cpp src/tyrx/*/*.cpp src/tyrx/*/*/*.cpp)
    OBJS += $(TYRX_SRCS:src/tyrx/%.cpp=obj/tyrx/%.o)
    DEPS += $(TYRX_SRCS:src/tyrx/%.cpp=obj/tyrx/%.d)
endif

###

ifeq ($(WITH_NETWORK), auto)
    ifeq ($(shell $(PKG_CONFIG) --exists SDL2_net 2>/dev/null && echo true), true)
        WITH_NETWORK := true
    else
        WITH_NETWORK := false
        $(info SDL2_net not found; building without network support)
    endif
endif

ifeq ($(WITH_NETWORK), true)
    EXTRA_CPPFLAGS += -DWITH_NETWORK
endif

ifeq ($(WITH_TYRX), true)
    EXTRA_LDFLAGS += -Wl,--wrap=SDL_GetTicks,--wrap=SDL_Delay,--wrap=time,--wrap=JE_itemScreen,--wrap=gameplaySelect,--wrap=JE_setupExplosion,--wrap=init_joysticks \
                     -Wl,--wrap=draw_background_1
    LINK = $(CXX) $(ALL_CXXFLAGS)
else
    LINK = $(CC) $(ALL_CFLAGS)
endif

OPENTYRIAN_VERSION := $(shell $(VCS_IDREV) 2>/dev/null && \
                              touch src/opentyrian_version.h)
ifneq ($(OPENTYRIAN_VERSION), )
    EXTRA_CPPFLAGS += -DOPENTYRIAN_VERSION='"$(OPENTYRIAN_VERSION)"'
endif

# -Wno-format-truncation only exists in GCC; Clang rejects it under -Werror
ifeq ($(findstring clang, $(shell $(CC) --version 2>/dev/null)), )
    WNO_FORMAT_TRUNCATION := -Wno-format-truncation
endif

CPPFLAGS ?= -MMD
CPPFLAGS += -DNDEBUG
CFLAGS ?= -pedantic \
          -Wall \
          -Wextra \
          $(WNO_FORMAT_TRUNCATION) \
          -Wno-missing-field-initializers \
          -O2
CXXFLAGS ?= -Wall \
            -Wextra \
            -Wno-missing-field-initializers \
            -O2
LDFLAGS ?=
LDLIBS ?=

ifeq ($(WITH_NETWORK), true)
    SDL_CPPFLAGS := $(shell $(PKG_CONFIG) sdl2 SDL2_net --cflags)
    SDL_LDFLAGS := $(shell $(PKG_CONFIG) sdl2 SDL2_net --libs-only-L --libs-only-other)
    SDL_LDLIBS := $(shell $(PKG_CONFIG) sdl2 SDL2_net --libs-only-l)
else
    SDL_CPPFLAGS := $(shell $(PKG_CONFIG) sdl2 --cflags)
    SDL_LDFLAGS := $(shell $(PKG_CONFIG) sdl2 --libs-only-L --libs-only-other)
    SDL_LDLIBS := $(shell $(PKG_CONFIG) sdl2 --libs-only-l)
endif

ALL_CPPFLAGS = -DTARGET_$(PLATFORM) \
               -DTYRIAN_DIR='"$(TYRIAN_DIR)"' \
               $(EXTRA_CPPFLAGS) \
               $(SDL_CPPFLAGS) \
               $(CPPFLAGS)
ALL_CFLAGS = -std=iso9899:1999 \
             $(CFLAGS)
ALL_CXXFLAGS = -std=c++26 -freflection \
               $(CXXFLAGS)
ALL_LDFLAGS = $(SDL_LDFLAGS) \
              $(EXTRA_LDFLAGS) \
              $(LDFLAGS)
ALL_LDLIBS = -lm \
             $(SDL_LDLIBS) \
             $(LDLIBS)

###

.PHONY : all
all : $(TARGET)

.PHONY : debug
debug : CPPFLAGS += -UNDEBUG
debug : CFLAGS += -Werror
debug : CFLAGS += -O0
debug : CFLAGS += -g3
debug : CXXFLAGS += -Werror
debug : CXXFLAGS += -O0
debug : CXXFLAGS += -g3
debug : all

.PHONY : installdirs
installdirs :
	mkdir -p $(DESTDIR)$(bindir)
	mkdir -p $(DESTDIR)$(docdir)
	mkdir -p $(DESTDIR)$(man6dir)
	mkdir -p $(DESTDIR)$(desktopdir)
	mkdir -p $(DESTDIR)$(icondir)/hicolor/22x22/apps
	mkdir -p $(DESTDIR)$(icondir)/hicolor/24x24/apps
	mkdir -p $(DESTDIR)$(icondir)/hicolor/32x32/apps
	mkdir -p $(DESTDIR)$(icondir)/hicolor/48x48/apps
	mkdir -p $(DESTDIR)$(icondir)/hicolor/128x128/apps

.PHONY : install
install : $(TARGET) installdirs
	$(INSTALL_PROGRAM) $(TARGET) $(DESTDIR)$(bindir)/
	$(INSTALL_DATA) README.md $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) linux/man/opentyrian.6 $(DESTDIR)$(man6dir)/opentyrian$(man6ext)
	$(INSTALL_DATA) linux/opentyrian.desktop $(DESTDIR)$(desktopdir)/
	$(INSTALL_DATA) linux/icons/tyrian-22.png $(DESTDIR)$(icondir)/hicolor/22x22/apps/opentyrian.png
	$(INSTALL_DATA) linux/icons/tyrian-24.png $(DESTDIR)$(icondir)/hicolor/24x24/apps/opentyrian.png
	$(INSTALL_DATA) linux/icons/tyrian-32.png $(DESTDIR)$(icondir)/hicolor/32x32/apps/opentyrian.png
	$(INSTALL_DATA) linux/icons/tyrian-48.png $(DESTDIR)$(icondir)/hicolor/48x48/apps/opentyrian.png
	$(INSTALL_DATA) linux/icons/tyrian-128.png $(DESTDIR)$(icondir)/hicolor/128x128/apps/opentyrian.png

.PHONY : uninstall
uninstall :
	rm -f $(DESTDIR)$(bindir)/$(TARGET)
	rm -f $(DESTDIR)$(docdir)/README.md
	rm -f $(DESTDIR)$(man6dir)/opentyrian$(man6ext)
	rm -f $(DESTDIR)$(desktopdir)/opentyrian.desktop
	rm -f $(DESTDIR)$(icondir)/hicolor/22x22/apps/opentyrian.png
	rm -f $(DESTDIR)$(icondir)/hicolor/24x24/apps/opentyrian.png
	rm -f $(DESTDIR)$(icondir)/hicolor/32x32/apps/opentyrian.png
	rm -f $(DESTDIR)$(icondir)/hicolor/48x48/apps/opentyrian.png
	rm -f $(DESTDIR)$(icondir)/hicolor/128x128/apps/opentyrian.png

.PHONY : clean
clean :
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(RES)
	rm -f $(TARGET)
	rm -rf obj/tyrx
	rm -f obj/.with_tyrx_*

$(TARGET) : $(OBJS) $(RES)
	$(LINK) $(ALL_LDFLAGS) -o $@ $(OBJS) $(RES) $(ALL_LDLIBS)

ifeq ($(WITH_TYRX), true)
# Check that every writable symbol of the C objects is classified.
TYRX_REGISTRY := src/tyrx/gold/registry.inc src/tyrx/gold/untracked.txt
$(TARGET) : | obj/tyrx/gold/registry.ok
obj/tyrx/gold/registry.ok : $(SRCS:src/%.c=obj/%.o) $(TYRX_REGISTRY) tools/check_c_state_registry.py
	NM=$(NM) CXX='$(CXX)' $(PYTHON) tools/check_c_state_registry.py $(TYRX_REGISTRY) $(SRCS:src/%.c=obj/%.o)
	@mkdir -p $(dir $@)
	@touch $@
endif

-include $(DEPS)

# Relink when WITH_TYRX changes, since it changes which objects are linked.
TYRX_STAMP := obj/.with_tyrx_$(WITH_TYRX)
$(TYRX_STAMP) :
	@mkdir -p "$(dir $@)"
	@rm -f obj/.with_tyrx_*
	@touch $@
$(TARGET) : $(TYRX_STAMP)

obj/%.o : src/%.c
	@mkdir -p "$(dir $@)"
	$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -c -o $@ $<

# -MP adds an empty rule for each header, so deleting one doesn't break the build.
obj/tyrx/%.o : src/tyrx/%.cpp
	@mkdir -p "$(dir $@)"
	$(CXX) $(ALL_CPPFLAGS) -MP $(ALL_CXXFLAGS) -c -o $@ $<

obj/resources.o : visualc/resources.rc visualc/tyrian.ico
	@mkdir -p "$(dir $@)"
	$(WINDRES) --include-dir visualc -i $< -o $@
