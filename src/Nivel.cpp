#include "Nivel.hpp"

#include <stdexcept>
#include <string>

Nivel::Nivel() {
    alturaPlataforma.fill(SIN_PLATAFORMA);
    peligro.fill(Peligro::Ninguno);
}

Conteos contarElementos(const Nivel& nivel) {
    Conteos c;
    for (int x = 0; x < ANCHO_NIVEL; ++x) {
        if (nivel.alturaPlataforma[x] != SIN_PLATAFORMA) ++c.plataformas;
        if (nivel.peligro[x] == Peligro::Enemigo) ++c.enemigos;
        if (nivel.peligro[x] == Peligro::Obstaculo) ++c.obstaculos;
    }
    return c;
}

void validarEstructura(const Nivel& nivel) {
    if (nivel.columnaInicio < 0 || nivel.columnaInicio >= ANCHO_NIVEL ||
        nivel.columnaMeta < 0 || nivel.columnaMeta >= ANCHO_NIVEL ||
        nivel.columnaInicio == nivel.columnaMeta) {
        throw std::runtime_error("las columnas de inicio y meta son invalidas");
    }

    for (int x = 0; x < ANCHO_NIVEL; ++x) {
        const int y = nivel.alturaPlataforma[x];
        const Peligro peligro = nivel.peligro[x];
        if (y < SIN_PLATAFORMA || y >= ALTO_NIVEL - 1) {
            throw std::runtime_error("plataforma fuera del tablero en columna " + std::to_string(x));
        }
        if (peligro != Peligro::Ninguno && (y == SIN_PLATAFORMA || y + 1 >= ALTO_NIVEL)) {
            throw std::runtime_error("peligro sin apoyo valido en columna " + std::to_string(x));
        }
    }

    const int yInicio = nivel.alturaPlataforma[nivel.columnaInicio];
    const int yMeta = nivel.alturaPlataforma[nivel.columnaMeta];
    if (yInicio == SIN_PLATAFORMA || yMeta == SIN_PLATAFORMA ||
        yInicio + 1 >= ALTO_NIVEL || yMeta + 1 >= ALTO_NIVEL) {
        throw std::runtime_error("inicio/meta sin superficie y espacio superior disponible");
    }
    if (nivel.peligro[nivel.columnaInicio] != Peligro::Ninguno ||
        nivel.peligro[nivel.columnaMeta] != Peligro::Ninguno) {
        throw std::runtime_error("inicio/meta estan ocupados por peligros");
    }
}
