#pragma once

#include "Instancia.hpp"
#include "Nivel.hpp"

#include <cstdint>

// Construccion inicial ALEATORIA, reproducible mediante una semilla.
// - mantiene inicio/meta en columnas 0/149 (hipotesis provisional);
// - elige P-2 columnas interiores distintas al azar;
// - asigna alturas aleatorias de 0 a 14;
// - distribuye E y O aleatoriamente en apoyos interiores distintos;
// - NO usa la linealidad solicitada para construir el nivel.
// Garantiza conteos y restricciones estructurales, no completabilidad.
Nivel generarInicial(const Instancia& instancia, std::uint32_t semilla);
