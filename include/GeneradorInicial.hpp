#pragma once

#include "Instancia.hpp"
#include "Nivel.hpp"

#include <cstdint>

// Construccion inicial ALEATORIA, reproducible mediante una semilla.
// - elige inicio en columnas 0..4 y meta en 145..149;
// - elige P-2 columnas adicionales sin reemplazo;
// - asigna alturas 0..15 (0..14 si hay personaje o peligro);
// - distribuye E y O aleatoriamente en apoyos distintos;
// - NO usa la linealidad solicitada para construir el nivel.
// Garantiza conteos y restricciones estructurales, no completabilidad.
Nivel generarInicial(const Instancia& instancia, std::uint32_t semilla);
