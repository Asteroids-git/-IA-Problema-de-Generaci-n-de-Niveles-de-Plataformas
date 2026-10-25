#include "GeneradorInicial.hpp"

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

Nivel generarInicial(const Instancia& instancia, std::uint32_t semilla) {
    validarInstancia(instancia);
    Nivel nivel;
    std::vector<int> columnas;
    columnas.reserve(instancia.plataformas);

    // Distribución determinista sobre toda la anchura. Los extremos son
    // plataformas de inicio y meta. Para 2 <= P <= 150 no hay duplicados.
    for (int i = 0; i < instancia.plataformas; ++i) {
        const int x = (i * (ANCHO_NIVEL - 1) + (instancia.plataformas - 1) / 2) /
                      (instancia.plataformas - 1);
        if (!columnas.empty() && columnas.back() == x) {
            throw std::logic_error("generacion de columnas duplicadas");
        }
        columnas.push_back(x);

        // Las alturas son propuestas iniciales, NO optimizadas.
        // Max: perfil horizontal. Min: alternancia moderada entre filas.
        const int y = instancia.objetivo == ObjetivoLinealidad::Maximizar
                          ? 4
                          : (i % 2 == 0 ? 4 : 7);
        nivel.alturaPlataforma[x] = y;
    }
    nivel.columnaInicio = columnas.front();
    nivel.columnaMeta = columnas.back();

    // Reservamos inicio/meta y mezclamos plataformas intermedias para
    // repartir peligros sin cambiar su cantidad ni solaparlos.
    std::vector<int> apoyos(columnas.begin() + 1, columnas.end() - 1);
    std::mt19937 rng(semilla);
    std::shuffle(apoyos.begin(), apoyos.end(), rng);

    std::size_t proximo = 0;
    for (int k = 0; k < instancia.enemigos; ++k) {
        nivel.peligro[apoyos.at(proximo++)] = Peligro::Enemigo;
    }
    for (int k = 0; k < instancia.obstaculos; ++k) {
        nivel.peligro[apoyos.at(proximo++)] = Peligro::Obstaculo;
    }

    validarEstructura(nivel);
    return nivel;
}
