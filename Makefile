CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude
FUENTES := $(wildcard src/*.cpp)
OBJETOS := $(patsubst src/%.cpp,build/%.o,$(FUENTES))
EJECUTABLE := plataformas
TEST_EJECUTABLE := build/prueba_movimiento
TEST_OBJETOS := $(filter-out build/main.o,$(OBJETOS))

.PHONY: all clean test
all: $(EJECUTABLE)

$(EJECUTABLE): $(OBJETOS)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/%.o: src/%.cpp $(wildcard include/*.hpp)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_EJECUTABLE): tests/prueba_movimiento.cpp $(TEST_OBJETOS)
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(EJECUTABLE) $(TEST_EJECUTABLE)
	bash tests/pruebas.sh
	./$(TEST_EJECUTABLE)

clean:
	rm -rf build $(EJECUTABLE)
