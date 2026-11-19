#pragma once

#include "Evaluador.hpp"

#include <cstdint>

struct ConfiguracionSA {
    int iteraciones = 3000;
    double temperaturaInicial = 0.08;
    double temperaturaFinal = 0.002;
    double probAvance = 0.45;
    double probAjuste = 0.20;
    PesosSA pesos;
};

struct EstadisticasSA {
    int iteraciones = 0;
    std::uint64_t evaluaciones = 0;
    std::uint64_t evaluacionesFactibles = 0;
    std::uint64_t evaluacionesConError = 0;
    int aceptadas = 0;
    int empeoramientosAceptados = 0;
    int primeraFactible = -1;
    int iteracionSeleccionada = 0;
    double costoInicial = 0.0;
    double progresoInicial = 0.0;
    double linealidadInicial = 0.0;
    double segundos = 0.0;
    double segundosVerificador = 0.0;
    double maxMilisegundosVerificador = 0.0;
};

struct ResultadoSA {
    Nivel nivel;
    Evaluacion evaluacion;
    ResultadoAlcanzabilidad ruta;
    EstadisticasSA estadisticas;
};

// Devuelve el mejor factible evaluado. Si ninguno es factible, devuelve el
// mejor candidato incompleto con ruta.completable == false.
ResultadoSA optimizarNivel(const Instancia& instancia, std::uint32_t semilla,
                            const ConfiguracionSA& configuracion = {});
ResultadoSA optimizarNivel(const Instancia& instancia, const Nivel& inicial, std::uint32_t semilla,
                            const ConfiguracionSA& configuracion = {});
