source = src/main.c

CFLAGS += -std=c99 -pedantic -Wall -Wextra
LDLIBS += -lm
LDLIBS += -lGL -lGLEW
LDLIBS += -lglfw

bin/attractors: $(source) | bin
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(LDLIBS)

bin:
	mkdir -p bin

clean:
	rm -rf bin

all: bin/attractors
