#pragma once

#include "Nivel.hpp"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <vector>

struct ResultadoAlcanzabilidad {
    bool completable = false;
    int apoyosSeguros = 0; // apoyos utilizables, incluidos los que requieren un pisoton
    int movimientosValidos = 0; // pares dirigidos con alguna trayectoria potencial
    int apoyosAlcanzados = 0;  // columnas distintas alcanzadas desde inicio
    std::vector<int> camino;  // columnas inicio -> meta, si existe
    std::bitset<ANCHO_NIVEL> columnasAlcanzadas;
};

// Contadores opcionales para el benchmark. Los estados cuentan la fase que
// conserva mascaras. El tiempo del grafo se mide en nanosegundos.
struct EstadisticasAlcanzabilidad {
    std::int64_t grafoNs = 0;
    std::size_t estadosExpandidos = 0;
    std::size_t estadosInsertados = 0;
    bool cotaCertifica = false;
};

// Parte del inicio con todos los enemigos vivos. El estado conserva la
// columna y los enemigos aplastados relevantes para la continuacion.
// Precondicion: el nivel cumple validarEstructura().
ResultadoAlcanzabilidad verificarCompletabilidad(const Nivel& nivel,
                                                 EstadisticasAlcanzabilidad* stats = nullptr);
