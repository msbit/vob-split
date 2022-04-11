CFLAGS += $(shell pkg-config --cflags dvdread)
LDLIBS += $(shell pkg-config --libs dvdread)
OBJECT_FILES = split.o vob-split.o
TARGETS = vob-split

all: $(TARGETS)

vob-split: $(OBJECT_FILES)

vob-split.o: vob-split.c split.h

split.o: split.c split.h

clean:
	rm -f $(OBJECT_FILES) $(TARGETS)
