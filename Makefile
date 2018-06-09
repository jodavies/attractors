source = src/main.c src/GetWallTime.c

CFLAGS += -pedantic -Wall -Wextra
CFLAGS += -O3 -g
LDLIBS += -lm
ifeq ($(shell uname -s),Linux)
	CFLAGS += -I/usr/include/freetype2
	LDLIBS += -lGL -lfreetype
else ifeq ($(shell uname -s),Darwin)
	CFLAGS += -DFOROSX
	LDLIBS += -framework OpenGL
endif
LDLIBS += -lGLEW -lglfw


bin/attractors: $(source) | bin
	$(CXX) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(LDLIBS)

bin:
	mkdir -p bin

clean:
	rm -rf bin

all: bin/attractors
