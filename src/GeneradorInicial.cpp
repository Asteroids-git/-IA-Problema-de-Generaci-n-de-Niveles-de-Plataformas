#include "GeneradorInicial.hpp"

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

Nivel generarInicial(const Instancia& instancia, std::uint32_t semilla) {
    validarInstancia(instancia);

    Nivel nivel;
    std::mt19937 rng(semilla);

    // Politica de diseño: inicio y meta dentro de bandas extremas disjuntas.
    // Distancia entre ambos >= 141 columnas; no es imposicion del enunciado.
    std::uniform_int_distribution<int> posInicio(0, 4);
    std::uniform_int_distribution<int> posMeta(ANCHO_NIVEL-5, ANCHO_NIVEL-1);
    nivel.columnaInicio = posInicio(rng);
    nivel.columnaMeta = posMeta(rng);
    std::vector<int> columnas = {nivel.columnaInicio, nivel.columnaMeta};
    columnas.reserve(instancia.plataformas);

    std::vector<int> disponibles;
    disponibles.reserve(ANCHO_NIVEL - 2);
    for (int x=0; x<ANCHO_NIVEL; ++x) {
        if (x != nivel.columnaInicio && x != nivel.columnaMeta)
            disponibles.push_back(x);
    }
    std::shuffle(disponibles.begin(), disponibles.end(), rng);
    columnas.insert(columnas.end(), disponibles.begin(),
                    disponibles.begin() + (instancia.plataformas - 2));

    // Se seleccionan primero las columnas de los peligros. Los apoyos que
    // alojan personaje/peligro requieren espacio arriba; el resto puede
    // contener plataformas en la fila 15, geometria no transitable.
    std::vector<int> apoyos(columnas.begin() + 2, columnas.end());
    std::shuffle(apoyos.begin(), apoyos.end(), rng);
    std::size_t siguiente = 0;
    for (int i = 0; i < instancia.enemigos; ++i)
        nivel.peligro[apoyos[siguiente++]] = Peligro::Enemigo;
    for (int i = 0; i < instancia.obstaculos; ++i)
        nivel.peligro[apoyos[siguiente++]] = Peligro::Obstaculo;

    std::uniform_int_distribution<int> alturaApoyo(0, ALTO_NIVEL - 2);
    std::uniform_int_distribution<int> alturaBloque(0, ALTO_NIVEL - 1);
    for (int x : columnas) {
        const bool extremo = x == nivel.columnaInicio || x == nivel.columnaMeta;
        const bool requiereEspacio = extremo || nivel.peligro[x] != Peligro::Ninguno;
        nivel.alturaPlataforma[x] = requiereEspacio ? alturaApoyo(rng) : alturaBloque(rng);
    }

    // Esta funcion garantiza estructura y conteos, no una ruta jugable.
    // La comprobacion de completabilidad corresponde a Alcanzabilidad.cpp.
    validarEstructura(nivel);
    return nivel;
}
