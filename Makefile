.POSIX:
.PHONY: all clean

CXX := g++ -std=c++17

CXXFLAGS := -fPIC -Wall -Werror -g
LDFLAGS := -shared -Wl,-soname,libmvke.so -lvulkan `pkg-config --static --libs glfw3`

BUILD_DIR := build

SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
HEADERS := $(wildcard *.hpp)
SHADERS := $(wildcard shaders/shader.*)
SHADOBJS := $(patsubst shaders/shader.%,$(BUILD_DIR)/shaders/%.spv,$(SHADERS))

all: $(BUILD_DIR)/libmvke.so $(SHADOBJS)

test: $(BUILD_DIR)/libmvke.so $(SHADOBJS)
	$(MAKE) -C test

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C test clean

$(BUILD_DIR)/libmvke.so: $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/shaders/%.spv: shaders/shader.%
	@mkdir -p $(dir $@)
	glslangValidator -V $< -o $@