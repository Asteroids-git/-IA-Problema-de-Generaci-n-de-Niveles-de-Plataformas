#pragma once

#include "Instancia.hpp"
#include "Nivel.hpp"

#include <cstdint>

// Construcción BASE: respeta cantidades, límites, apoyo y no superposición.
// No garantiza por construccion alcanzar la meta: la verificacion se hace
// despues mediante Movimiento + Alcanzabilidad (BFS).
Nivel generarInicial(const Instancia& instancia, std::uint32_t semilla);
