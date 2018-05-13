source = src/main.c src/GetWallTime.c

CFLAGS += -std=c99 -pedantic -Wall -Wextra
CFLAGS += -O3
LDLIBS += -lm -fopenmp
LDLIBS += -lGL -lGLEW
LDLIBS += -lglfw

bin/attractors: $(source) | bin
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(LDLIBS)

bin:
	mkdir -p bin

clean:
	rm -rf bin

all: bin/attractors
