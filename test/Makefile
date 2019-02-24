.POSIX:
.PHONY: all clean

CXXFLAGS := -std=c++17 -Wall -Werror -g
LDFLAGS := -L../build -lmvke -lvulkan

SOURCE := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCE))

all: test

clean:
	rm -f test
	rm -f *.o

test: $(OBJECTS)
	g++ -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	g++ $(CXXFLAGS) -c $<
