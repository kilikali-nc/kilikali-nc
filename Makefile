RM=rm -rf
MKDIR=mkdir -p
INSTALL=install -D
ECHO=echo
SED=sed

PREFIX?=/usr/local
ROOT_DIR:=$(dir $(realpath $(firstword $(MAKEFILE_LIST))))
BUILD_DIR?=$(realpath .)

GST_CFLAGS=`pkg-config --cflags gstreamer-1.0`
NCURSES_CFLAGS=`pkg-config --cflags ncursesw`
CURL_CFLAGS=`curl-config --cflags`
CFLAGS+=$(GST_CFLAGS) $(NCURSES_CFLAGS) $(CURL_CFLAGS) -I$(ROOT_DIR) -I$(ROOT_DIR)/src -I$(BUILD_DIR)/h -Wall -Werror

GST_LIBS=`pkg-config --libs gstreamer-1.0`
NCURSES_LIBS=`pkg-config --libs ncursesw`
CURL_LIBS=`curl-config --libs`
LIBMAGIC_LIBS=-lmagic
LDFLAGS+=$(GST_LIBS) $(NCURSES_LIBS) $(CURL_LIBS) $(LIBMAGIC_LIBS)

TARGET=$(BUILD_DIR)/kilikali-nc
VERSION=1.0
MANPAGE=$(BUILD_DIR)/kilikali-nc.1
PACKAGE=$(TARGET)
LOCALEDIR?=$(PREFIX)/share/locale
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man/man1
APP_EXE=$(notdir $(TARGET))

APP_CFLAGS = -DAPP_EXE=\"$(APP_EXE)\" -DPACKAGE=\"$(PACKAGE)\" -DLOCALEDIR=\"$(LOCALEDIR)\"
CFLAGS += $(APP_CFLAGS)

DEPS =	src/ncurses-event.h \
	src/ncurses-common.h \
	src/ncurses-colors.h \
	src/ncurses-screen.h \
	src/ncurses-key-sequence.h \
	src/ncurses-scroller.h \
	src/ncurses-subwindow-textview.h \
	src/ncurses-window-command-prompt.h \
	src/ncurses-window-user-info.h \
	src/ncurses-window-playlist.h \
	src/ncurses-window-filebrowser.h \
	src/ncurses-window-help.h \
	src/ncurses-window-lyrics.h \
	src/ncurses-window-volume-and-mode.h \
	src/ncurses-window-time.h \
	src/ncurses-window-info.h \
	src/ncurses-window-title.h \
	src/ncurses-window-error.h \
	src/player.h \
	src/inspector.h \
	src/gst/typefind-hack.h \
	src/gst/common.h \
	src/playlist-line.h \
	src/playlist.h \
	src/playlist-pls.h \
	src/playlist-m3u.h \
	src/paths.h \
	src/song.h \
	src/net.h \
	src/net-common.h \
	src/net-lyrics.h \
	src/keys.h \
	src/cmdline.h \
	src/cmdline-mode.h \
	src/config.h \
	src/util.h \
	src/command.h \
	src/commands.h \
	src/log.h \
	src/search.h \
	src/sid.h \
	$(BUILD_DIR)/h/help.h

SRCS =	src/main.c \
	src/song.c \
	src/ncurses-event.c \
	src/ncurses-colors.c \
	src/ncurses-screen.c \
	src/ncurses-key-sequence.c \
	src/ncurses-scroller.c \
	src/ncurses-subwindow-textview.c \
	src/ncurses-window-command-prompt.c \
	src/ncurses-window-user-info.c \
	src/ncurses-window-playlist.c \
	src/ncurses-window-filebrowser.c \
	src/ncurses-window-help.c \
	src/ncurses-window-lyrics.c \
	src/ncurses-window-volume-and-mode.c \
	src/ncurses-window-time.c \
	src/ncurses-window-info.c \
	src/ncurses-window-title.c \
	src/ncurses-window-error.c \
	src/gst/common.c \
	src/gst/typefind-hack.c \
	src/gst/player.c \
	src/gst/inspector.c \
	src/playlist-line.c \
	src/playlist.c \
	src/playlist-pls.c \
	src/playlist-m3u.c \
	src/paths.c \
	src/net.c \
	src/net-common.c \
	src/net-lyrics.c \
	src/net-lyrics-chartlyrics.c \
	src/keys.c \
	src/cmdline.c \
	src/config.c \
	src/util.c \
	src/command.c \
	src/log.c \
	src/search.c \
	src/sid.c

OBJS = $(subst src/,$(BUILD_DIR)/objs/,$(SRCS:.c=.o))

all:
	echo $(BUILD_DIR)
	echo $(ROOT_DIR)
all: release

release: CFLAGS += -DNDEBUG -DREDIRECT_STDERR_TO_FILE -O2
release: LDFLAGS += -O2
release: xdeps $(TARGET) man

debug: CFLAGS += -DDEBUG -g -O0
# -fsanitize=address
debug: LDFLAGS += -g -O0
# -fsanitize=address
debug: xdeps $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

xdeps: $(BUILD_DIR)/h/help.h $(BUILD_DIR)/objs/gst

$(BUILD_DIR)/objs/gst:
	$(MKDIR) $(BUILD_DIR)/objs/gst

$(BUILD_DIR)/h:
	$(MKDIR) $(BUILD_DIR)/h

$(BUILD_DIR)/h/help.h: $(ROOT_DIR)/doc/kilikali-nc.txt $(BUILD_DIR)/h
	$(ECHO) "#ifndef KK_NCURSES_HELP_H" > $@
	$(ECHO) "#define KK_NCURSES_HELP_H" >> $@
	$(ECHO) "const char help_str[] = \"\\" >> $@
	$(SED) -r 's/$$/\\n\\/' $< >> $@
	$(ECHO) "\";" >> $@
	$(ECHO) "#endif" >> $@

$(OBJS): $(BUILD_DIR)/objs/%.o : $(ROOT_DIR)/src/%.c 
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: man	
man: $(ROOT_DIR)/doc/kilikali-nc.txt
	txt2man -t$(TARGET) -smultimedia -s1 -r$(TARGET)-$(VERSION) -vmultimedia $^ | gzip > $(MANPAGE)

.PHONY: install
install: install_bin install_locales install_man

.PHONY: install_bin
install_bin:
	$(INSTALL) $(TARGET) -t $(BINDIR)

.PHONY: install_locales
install_locales:
	$(INSTALL) $(ROOT_DIR)/locale/fi/LC_MESSAGES/$(APP_EXE).mo -t $(LOCALEDIR)/fi/LC_MESSAGES/

.PHONY: install_man
install_man:
	$(INSTALL) $(MANPAGE) -t $(MANDIR)

.PHONY: clean
clean:
	$(RM) $(BUILD_DIR)/objs $(TARGET)

.PHONY: distclean
distclean: clean
	$(RM) $(MANPAGE) $(BUILD_DIR)/h

