.POSIX:
.PHONY: all clean

CXXFLAGS := -std=c++17 -Wall -Werror -g -O0
LDFLAGS := -L../build -lmvke -lvulkan

BUILD_DIR := build

SOURCE := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCE))
HEADERS := $(wildcard *.hpp)

all: $(BUILD_DIR)/test

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR)/test: $(OBJECTS)
	@mkdir -p $(dir $@)
	g++ -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -c $< -o $@
