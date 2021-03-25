CFLAGS += $(shell pkg-config --cflags dvdread)
LDLIBS += $(shell pkg-config --libs dvdread)
OBJECT_FILES = main.o split.o
TARGETS = main

all: main

main: $(OBJECT_FILES)

main.o: main.c split.h

split.o: split.c split.h

clean:
	rm -f $(OBJECT_FILES) $(TARGETS)
