source = src/main.c src/GetWallTime.c

CFLAGS += -std=c99 -pedantic -Wall -Wextra
CFLAGS += -O3
CFLAGS += -D_POSIX_C_SOURCE=200112L
LDLIBS += -lm
ifeq ($(shell uname -s),Linux)
	LDLIBS += -lGL
else ifeq ($(shell uname -s),Darwin)
	CFLAGS += -DFOROSX
	LBLIBS += -framework OpenGL
endif
LDLIBS += -lGLEW -lglfw


bin/attractors: $(source) | bin
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(LDLIBS)

bin:
	mkdir -p bin

clean:
	rm -rf bin

all: bin/attractors
