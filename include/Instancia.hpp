#pragma once

#include <string>

// Datos de entrada de un caso de prueba. No contiene variables de decisión.
enum class ObjetivoLinealidad { Maximizar, Minimizar };

struct Instancia {
    int id = 0;
    int plataformas = 0;
    int enemigos = 0;
    int obstaculos = 0;
    ObjetivoLinealidad objetivo = ObjetivoLinealidad::Maximizar;
};

std::string nombreObjetivo(ObjetivoLinealidad objetivo);

// Reglas básicas de capacidad de nuestra representación inicial.
// No reemplaza el validador de jugabilidad del nivel.
void validarInstancia(const Instancia& instancia);
