#pragma once

#include "Nivel.hpp"
#include <vector>

struct ResultadoAlcanzabilidad {
    bool completable = false;
    int apoyosSeguros = 0;
    int movimientosValidos = 0; // aristas dirigidas calculadas
    int apoyosAlcanzados = 0;  // desde inicio
    std::vector<int> camino;  // columnas inicio -> meta, si existe
};

// Construye implicitamente el grafo dirigido y hace BFS.
// NO exige que todas las plataformas sean alcanzables, solo la meta.
ResultadoAlcanzabilidad verificarCompletabilidad(const Nivel& nivel);
