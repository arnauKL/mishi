TARGET=mishi

SOURCES=mishi.c

all: $(SOURCES)
	cc $(SOURCES) -o $(TARGET)


.PHONY: clean

clean:
	rm $(TARGET)
