#pragma once

#include "Nivel.hpp"

// Movimiento simplificado ACORDADO para el avance (no motor de fisica):
// - personaje de 1x1 celda, con pies en el borde superior del bloque;
// - despegue y detencion horizontal en el centro de las columnas;
// - alturas de salto h = 4,3,2,1 con r(h) = 1.5*h;
// - parabola de los pies: y(d) = y0 + 4*h*(d/r)*(1-d/r);
// - en la columna de destino se puede parar horizontalmente y caer;
// - altura maxima 4, avance horizontal maximo 6;
// - contacto con bordes admitido; penetracion de celdas prohibida.
// Ninguna de estas simplificaciones constituye una regla fisica literal del PPT.

// Determina si existe uno de los cuatro saltos definidos que conecte
// el centro de dos columnas con plataformas seguras y evite colisiones.
bool saltoValido(const Nivel& nivel, int origen, int destino);

// Caminar horizontalmente entre los centros de plataformas contiguas
// a la misma altura, sin cruzar peligros.
bool caminarValido(const Nivel& nivel, int origen, int destino);

// Retorna true si existe un movimiento directo (caminar o saltar).
bool movimientoDirectoValido(const Nivel& nivel, int origen, int destino);
