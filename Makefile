CXX ?= c++
BUILD ?= build
CPPFLAGS += -Iinclude -Ivendor/SDL2
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic
CORE = src/workers.cpp src/director.cpp src/world.cpp src/generation.cpp src/ecology.cpp src/animals.cpp src/buildings.cpp src/population.cpp src/brain.cpp src/learning.cpp src/occupations.cpp src/society.cpp src/saves.cpp src/slots.cpp
CORE_OBJECTS = $(patsubst src/%.cpp,$(BUILD)/core/%.o,$(CORE))
GAME = $(CORE_OBJECTS) src/main.cpp src/port_art.cpp src/performance.cpp src/sound.cpp src/feedback.cpp src/sdl.cpp src/input.cpp src/rumble.cpp src/render.cpp src/sprites.cpp src/ui.cpp
HEADERS = $(wildcard include/living_village/*.hpp)

.PHONY: all test input-test render-test feedback-test game-smoke worlds-smoke world-soak clean package
all: $(BUILD)/living-village

$(BUILD):
	mkdir -p $@

$(BUILD)/living-village: $(GAME) $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(GAME) -o $@ $(LDFLAGS) -ldl -pthread

$(BUILD)/simulation-tests: tests/simulation_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/simulation_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/development-tests: tests/development_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/development_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/population-tests: tests/population_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/population_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/learning-tests: tests/learning_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/learning_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/adaptation-tests: tests/adaptation_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/adaptation_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/society-tests: tests/society_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/society_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/world-management-tests: tests/world_management_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/world_management_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/ecology-tests: tests/ecology_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/ecology_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

test: $(BUILD)/workers-tests $(BUILD)/director-tests $(BUILD)/lively-tests $(BUILD)/rumble-tests $(BUILD)/telemetry-tests $(BUILD)/port-art-tests $(BUILD)/simulation-tests $(BUILD)/development-tests $(BUILD)/population-tests $(BUILD)/learning-tests $(BUILD)/adaptation-tests $(BUILD)/society-tests $(BUILD)/world-management-tests $(BUILD)/ecology-tests
	./$(BUILD)/workers-tests
	./$(BUILD)/director-tests
	./$(BUILD)/lively-tests
	./$(BUILD)/rumble-tests
	./$(BUILD)/telemetry-tests
	./$(BUILD)/port-art-tests
	sh tests/diagnostics_test.sh
	./$(BUILD)/simulation-tests
	./$(BUILD)/development-tests
	./$(BUILD)/population-tests
	./$(BUILD)/learning-tests
	./$(BUILD)/adaptation-tests
	./$(BUILD)/society-tests
	./$(BUILD)/world-management-tests
	./$(BUILD)/ecology-tests

$(BUILD)/input-tests: tests/input_test.cpp src/input.cpp src/rumble.cpp src/sdl.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/input_test.cpp src/input.cpp src/rumble.cpp src/sdl.cpp -o $@ $(LDFLAGS) -ldl -pthread

input-test: $(BUILD)/input-tests
	./$(BUILD)/input-tests

$(BUILD)/render-tests: tests/render_test.cpp $(CORE_OBJECTS) src/render.cpp src/sprites.cpp src/sdl.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/render_test.cpp $(CORE_OBJECTS) src/render.cpp src/sprites.cpp src/sdl.cpp -o $@ $(LDFLAGS) -ldl -pthread

render-test: $(BUILD)/render-tests
	SDL_VIDEODRIVER=dummy ./$(BUILD)/render-tests

$(BUILD)/feedback-tests: tests/feedback_test.cpp src/feedback.cpp src/sound.cpp src/performance.cpp src/input.cpp src/rumble.cpp src/sdl.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/feedback_test.cpp src/feedback.cpp src/sound.cpp src/performance.cpp src/input.cpp src/rumble.cpp src/sdl.cpp -o $@ $(LDFLAGS) -ldl -pthread

feedback-test: $(BUILD)/feedback-tests
	SDL_AUDIODRIVER=dummy ./$(BUILD)/feedback-tests

game-smoke: $(BUILD)/living-village
	./scripts/smoke_desktop.sh ./$(BUILD)/living-village

worlds-smoke: $(BUILD)/living-village
	./scripts/smoke_worlds.sh ./$(BUILD)/living-village

world-soak: $(BUILD)/living-village
	./scripts/soak_worlds.sh ./$(BUILD)/living-village

package:
	./scripts/package_knulli.sh

clean:
	rm -rf $(BUILD)

$(BUILD)/port-art-tests: tests/port_art_test.cpp src/port_art.cpp include/living_village/port_art.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/port_art_test.cpp src/port_art.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/telemetry-tests: tests/telemetry_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/telemetry_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/lively-tests: tests/lively_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/lively_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/rumble-tests: tests/rumble_test.cpp src/rumble.cpp include/living_village/rumble.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/rumble_test.cpp src/rumble.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/director-tests: tests/director_test.cpp $(CORE_OBJECTS) src/ui.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/director_test.cpp $(CORE_OBJECTS) src/ui.cpp -o $@ $(LDFLAGS) -pthread

director-test: $(BUILD)/workers-tests $(BUILD)/director-tests
	./$(BUILD)/director-tests

$(BUILD)/workers-tests: tests/workers_test.cpp src/workers.cpp include/living_village/workers.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/workers_test.cpp src/workers.cpp -o $@ $(LDFLAGS) -pthread

$(BUILD)/core/%.o: src/%.cpp $(HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
