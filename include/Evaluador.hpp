#pragma once

#include "Instancia.hpp"
#include "Nivel.hpp"

struct Evaluacion {
    Conteos conteos;
    int errorCantidades = 0;
    double linealidad = 0;
    double penalizacionLinealidad = 0;
    double costoTotal = 0;
};

// Evaluación de CALIDAD para el enfoque inicial. No certifica jugabilidad.
Evaluacion evaluar(const Nivel& nivel, const Instancia& instancia);
