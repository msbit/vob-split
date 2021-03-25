CFLAGS += $(shell pkg-config --cflags dvdread)
LDLIBS += $(shell pkg-config --libs dvdread)
OBJECT_FILES = main.o
TARGETS = main

all: main

main: $(OBJECT_FILES)

clean:
	rm -f $(OBJECT_FILES) $(TARGETS)
