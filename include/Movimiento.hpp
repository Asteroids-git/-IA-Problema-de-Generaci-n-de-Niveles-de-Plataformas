#pragma once

#include "Nivel.hpp"

#include <bitset>
#include <vector>

constexpr int ALCANCE_MAXIMO_MOVIMIENTO = 6;
using EnemigosAplastados = std::bitset<ANCHO_NIVEL>;

// Una trayectoria puede exigir que ciertos enemigos ya hayan desaparecido y
// aplastar a otros durante el descenso. Cada bit identifica una columna.
struct CondicionMovimiento {
    EnemigosAplastados requiere;
    EnemigosAplastados aplasta;

    bool habilitada(const EnemigosAplastados& enemigos) const {
        return (requiere & ~enemigos).none();
    }
};

// Los movimientos parten de un apoyo centrado y terminan centrados. El salto
// sube de 1 a 4 celdas y la caida empieza al perder el apoyo, sin ascenso.
// Se permite aterrizar en el apice de altura maxima sobre una plataforma
// solida al alcanzar su borde con trayectoria libre. El pisoton exige
// velocidad vertical descendente y solapamiento horizontal al contacto.
// Por defecto todos los enemigos estan vivos, incluido el del origen.
bool saltoValido(const Nivel& nivel, int origen, int destino,
                 const EnemigosAplastados& enemigos = {});
bool caidaValida(const Nivel& nivel, int origen, int destino,
                 const EnemigosAplastados& enemigos = {});
bool caminarValido(const Nivel& nivel, int origen, int destino,
                   const EnemigosAplastados& enemigos = {});
bool movimientoDirectoValido(const Nivel& nivel, int origen, int destino,
                            const EnemigosAplastados& enemigos = {});

// Opciones de caminar, caida y salto para un par de apoyos. Se descartan
// trayectorias con mas requisitos que otra que deja al menos los mismos
// enemigos eliminados al llegar.
std::vector<CondicionMovimiento> condicionesMovimiento(const Nivel& nivel, int origen, int destino);
