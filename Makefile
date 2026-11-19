CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude
FUENTES := $(wildcard src/*.cpp)
OBJETOS := $(patsubst src/%.cpp,build/%.o,$(FUENTES))
EJECUTABLE := plataformas
TEST_EJECUTABLE := build/prueba_movimiento
TEST_GENERADOR := build/prueba_generador
TEST_SA := build/prueba_sa
TEST_OBJETOS := $(filter-out build/main.o,$(OBJETOS))
BENCH_OBJETOS := $(addprefix build/,Alcanzabilidad.o Movimiento.o Nivel.o Instancia.o GeneradorInicial.o LectorInstancias.o)

.PHONY: all clean test benchmark
all: $(EJECUTABLE)

$(EJECUTABLE): $(OBJETOS)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/%.o: src/%.cpp $(wildcard include/*.hpp)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_EJECUTABLE): tests/prueba_movimiento.cpp $(TEST_OBJETOS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_GENERADOR): tests/prueba_generador.cpp $(TEST_OBJETOS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_SA): tests/prueba_sa.cpp $(TEST_OBJETOS)
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(EJECUTABLE) $(TEST_EJECUTABLE) $(TEST_GENERADOR) $(TEST_SA)
	bash tests/pruebas.sh
	./$(TEST_EJECUTABLE)
	./$(TEST_GENERADOR)
	./$(TEST_SA)

build/medir_alcanzabilidad: benchmarks/medir_alcanzabilidad.cpp $(BENCH_OBJETOS)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/medir_alcanzabilidad_heap: benchmarks/medir_alcanzabilidad.cpp $(BENCH_OBJETOS)
	$(CXX) $(CXXFLAGS) -DMEDIR_HEAP $^ -o $@

benchmark: build/medir_alcanzabilidad build/medir_alcanzabilidad_heap
	python3 benchmarks/ejecutar.py

clean:
	rm -rf build $(EJECUTABLE)
