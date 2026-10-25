#pragma once

#include <array>

constexpr int ANCHO_NIVEL = 150;
constexpr int ALTO_NIVEL = 16;
constexpr int SIN_PLATAFORMA = -1;

// Enemigos y obstáculos se contabilizan por separado, pero por ahora
// tienen el mismo comportamiento geométrico: peligros estáticos de 1 celda.
enum class Peligro { Ninguno, Enemigo, Obstaculo };

struct Nivel {
    // alturaPlataforma[x] == -1 significa que no existe bloque en la columna x.
    // En otro caso, guarda la fila del bloque (0..14, reservando la fila superior
    // para poder apoyar un personaje o peligro de altura 1).
    // Esta representación asegura como máximo una plataforma por columna.
    std::array<int, ANCHO_NIVEL> alturaPlataforma{};

    // Si existe peligro en x, ocupa (x, alturaPlataforma[x] + 1).
    std::array<Peligro, ANCHO_NIVEL> peligro{};

    int columnaInicio = 0;
    int columnaMeta = ANCHO_NIVEL - 1;

    Nivel();
};

struct Conteos {
    int plataformas = 0;
    int enemigos = 0;
    int obstaculos = 0;
};

Conteos contarElementos(const Nivel& nivel);

// Verifica solo restricciones estructurales, NO existencia de ruta jugable.
// Lanza std::runtime_error con un mensaje concreto si encuentra un fallo.
void validarEstructura(const Nivel& nivel);
