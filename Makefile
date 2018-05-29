source = src/main.c src/GetWallTime.c

CFLAGS += -pedantic -Wall -Wextra
CFLAGS += -O3
LDLIBS += -lm
ifeq ($(shell uname -s),Linux)
	LDLIBS += -lGL
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
