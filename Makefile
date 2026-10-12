CXX ?= c++
BUILD ?= build
CPPFLAGS += -Iinclude -Ivendor/SDL2
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic
CORE = src/world.cpp src/buildings.cpp src/population.cpp src/brain.cpp src/learning.cpp src/saves.cpp
GAME = $(CORE) src/main.cpp src/sdl.cpp src/input.cpp src/render.cpp src/ui.cpp
HEADERS = $(wildcard include/living_village/*.hpp)

.PHONY: all test input-test render-test clean package
all: $(BUILD)/living-village

$(BUILD):
	mkdir -p $@

$(BUILD)/living-village: $(GAME) $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(GAME) -o $@ $(LDFLAGS) -ldl -pthread

$(BUILD)/simulation-tests: tests/simulation_test.cpp $(CORE) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/simulation_test.cpp $(CORE) src/ui.cpp -o $@ $(LDFLAGS)

$(BUILD)/development-tests: tests/development_test.cpp $(CORE) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/development_test.cpp $(CORE) src/ui.cpp -o $@ $(LDFLAGS)

$(BUILD)/population-tests: tests/population_test.cpp $(CORE) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/population_test.cpp $(CORE) src/ui.cpp -o $@ $(LDFLAGS)

$(BUILD)/learning-tests: tests/learning_test.cpp $(CORE) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/learning_test.cpp $(CORE) src/ui.cpp -o $@ $(LDFLAGS)

test: $(BUILD)/simulation-tests $(BUILD)/development-tests $(BUILD)/population-tests $(BUILD)/learning-tests
	./$(BUILD)/simulation-tests
	./$(BUILD)/development-tests
	./$(BUILD)/population-tests
	./$(BUILD)/learning-tests

$(BUILD)/input-tests: tests/input_test.cpp src/input.cpp src/sdl.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/input_test.cpp src/input.cpp src/sdl.cpp -o $@ $(LDFLAGS) -ldl

input-test: $(BUILD)/input-tests
	./$(BUILD)/input-tests

$(BUILD)/render-tests: tests/render_test.cpp $(CORE) src/render.cpp src/sdl.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/render_test.cpp $(CORE) src/render.cpp src/sdl.cpp -o $@ $(LDFLAGS) -ldl

render-test: $(BUILD)/render-tests
	SDL_VIDEODRIVER=dummy ./$(BUILD)/render-tests

package:
	./scripts/package_knulli.sh

clean:
	rm -rf $(BUILD)
