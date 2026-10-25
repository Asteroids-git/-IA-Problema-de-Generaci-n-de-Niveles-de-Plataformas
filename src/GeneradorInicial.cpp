#include "GeneradorInicial.hpp"

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

Nivel generarInicial(const Instancia& instancia, std::uint32_t semilla) {
    validarInstancia(instancia);

    Nivel nivel;
    std::mt19937 rng(semilla);

    // Reservamos dos apoyos para el inicio y la meta. Su posicion en las
    // columnas extremas es una hipotesis del modelo, NO una regla oficial.
    nivel.columnaInicio = 0;
    nivel.columnaMeta = ANCHO_NIVEL - 1;
    std::vector<int> columnas = {nivel.columnaInicio, nivel.columnaMeta};
    columnas.reserve(instancia.plataformas);

    // Seleccion uniforme de columnas interiores sin reemplazo. Ni el numero
    // de columna ni la altura dependen del objetivo (max/min linealidad).
    std::vector<int> disponibles(ANCHO_NIVEL - 2);
    std::iota(disponibles.begin(), disponibles.end(), 1);
    std::shuffle(disponibles.begin(), disponibles.end(), rng);
    columnas.insert(columnas.end(), disponibles.begin(),
                    disponibles.begin() + (instancia.plataformas - 2));

    // Una altura independiente y aleatoria por plataforma. La ultima fila
    // (15) se reserva como espacio para el personaje o peligro apoyado.
    std::uniform_int_distribution<int> altura(0, ALTO_NIVEL - 2);
    for (int x : columnas) {
        nivel.alturaPlataforma[x] = altura(rng);
    }

    // Los peligros tambien se distribuyen aleatoriamente, sin reemplazo.
    // No pueden colocarse sobre inicio/meta, ni puede haber dos en un apoyo.
    std::vector<int> apoyos(columnas.begin() + 2, columnas.end());
    std::shuffle(apoyos.begin(), apoyos.end(), rng);
    std::size_t siguiente = 0;
    for (int i = 0; i < instancia.enemigos; ++i) {
        nivel.peligro[apoyos[siguiente++]] = Peligro::Enemigo;
    }
    for (int i = 0; i < instancia.obstaculos; ++i) {
        nivel.peligro[apoyos[siguiente++]] = Peligro::Obstaculo;
    }

    // Esta funcion garantiza estructura y conteos, no una ruta jugable.
    // La comprobacion de completabilidad corresponde a Alcanzabilidad.cpp.
    validarEstructura(nivel);
    return nivel;
}
