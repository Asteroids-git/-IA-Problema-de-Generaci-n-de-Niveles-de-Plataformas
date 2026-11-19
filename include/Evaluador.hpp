#pragma once

#include "Alcanzabilidad.hpp"
#include "Instancia.hpp"
#include "Nivel.hpp"

struct Evaluacion {
    Conteos conteos;
    int errorCantidades = 0;
    double linealidad = 0;
    double penalizacionLinealidad = 0;
    double costoTotal = 0;
    double progreso = 0; // solo se calcula en la evaluacion para SA
};

struct PesosSA {
    double cantidades = 0.07;
    double linealidad = 0.5;
    double incompletabilidad = 1.5;
    double progreso = 3.0;
};

// Evaluación de CALIDAD para el enfoque inicial. No certifica jugabilidad.
Evaluacion evaluar(const Nivel& nivel, const Instancia& instancia);

// La ruta debe corresponder al nivel evaluado por el verificador exacto.
Evaluacion evaluarParaSA(const Nivel& nivel, const Instancia& instancia,
                         const ResultadoAlcanzabilidad& ruta, const PesosSA& pesos = {});
