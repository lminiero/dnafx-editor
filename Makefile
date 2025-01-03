CC = gcc
STUFF = $(shell pkg-config --cflags glib-2.0 libusb-1.0 jansson) -D_GNU_SOURCE
STUFF_LIBS = $(shell pkg-config --libs glib-2.0 libusb-1.0 jansson)
OPTS = -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wunused #-Werror #-O2
GDB = -g -ggdb
#~ ASAN = -O1 -g3 -ggdb3 -fno-omit-frame-pointer -fsanitize=address -fno-sanitize-recover=all -fsanitize-address-use-after-scope
#~ ASAN_LIBS = -fsanitize=address

DNAFX_EDITOR = dnafx-editor
DNAFX_EDITOR_OBJS = src/dnafx-editor.o src/options.o \
	src/usb.o src/tasks.o src/presets.o src/utils.o src/embedded_cli.o

all: $(DNAFX_EDITOR)

DEPS := $(DNAFX_EDITOR_OBJS:.o=.d)
-include $(DEPS)

%.o: %.c
	$(CC) $(ASAN) $(STUFF) -fPIC $(GDB) -MMD -c $< -o $@ $(OPTS)

$(DNAFX_EDITOR): $(DNAFX_EDITOR_OBJS)
	$(CC) $(GDB) -o $(DNAFX_EDITOR) $(DNAFX_EDITOR_OBJS) $(ASAN_LIBS) $(STUFF_LIBS)

clean:
	rm -f $(DNAFX_EDITOR) src/*.o src/*.d

.PHONY: all clean
