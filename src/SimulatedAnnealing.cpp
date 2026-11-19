#include "SimulatedAnnealing.hpp"
#include "GeneradorInicial.hpp"
#include "Vecindario.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>
#include <stdexcept>
#include <utility>

namespace {
using Reloj = std::chrono::steady_clock;

struct Candidato {
    Nivel nivel;
    Evaluacion evaluacion;
    ResultadoAlcanzabilidad ruta;
};

void validarConfiguracion(const ConfiguracionSA& config) {
    if (config.iteraciones < 0 || !std::isfinite(config.temperaturaInicial) ||
        !std::isfinite(config.temperaturaFinal) || config.temperaturaFinal <= 0 ||
        config.temperaturaInicial < config.temperaturaFinal) {
        throw std::invalid_argument("SA requiere iteraciones no negativas y temperaturas finitas con inicial >= final > 0");
    }
    if (!std::isfinite(config.probAvance + config.probAjuste) || config.probAvance < 0 ||
        config.probAjuste < 0 || config.probAvance + config.probAjuste > 1) {
        throw std::invalid_argument("las probabilidades dirigidas de SA deben ser no negativas y sumar a lo sumo 1");
    }
    const auto& p = config.pesos;
    if (p.cantidades < 0 || p.linealidad < 0 || p.incompletabilidad < 0 || p.progreso < 0 ||
        !std::isfinite(3 * ANCHO_NIVEL * p.cantidades + p.linealidad + p.incompletabilidad + p.progreso)) {
        throw std::invalid_argument("los pesos de SA deben ser finitos, no negativos y mantener un costo finito");
    }
}

Candidato evaluarCandidato(Nivel nivel, const Instancia& instancia, const PesosSA& pesos,
                             EstadisticasSA& stats, int iteracion) {
    validarEstructura(nivel);
    const auto inicio = Reloj::now();
    auto ruta = verificarCompletabilidad(nivel);
    const double segundos = std::chrono::duration<double>(Reloj::now() - inicio).count();
    stats.segundosVerificador += segundos;
    stats.maxMilisegundosVerificador = std::max(stats.maxMilisegundosVerificador, 1000 * segundos);
    ++stats.evaluaciones;
    if (ruta.completable) {
        ++stats.evaluacionesFactibles;
        if (stats.primeraFactible < 0) stats.primeraFactible = iteracion;
    }
    auto evaluacion = evaluarParaSA(nivel, instancia, ruta, pesos);
    if (evaluacion.errorCantidades > 0) ++stats.evaluacionesConError;
    return {std::move(nivel), evaluacion, std::move(ruta)};
}

bool mejoraLaSeleccion(const Candidato& candidato, const Candidato& mejor) {
    if (candidato.ruta.completable != mejor.ruta.completable) return candidato.ruta.completable;
    return candidato.evaluacion.costoTotal < mejor.evaluacion.costoTotal;
}

ResultadoSA buscar(const Instancia& instancia, const Nivel& inicial, std::uint32_t semilla,
                    const ConfiguracionSA& config, Reloj::time_point inicio) {
    validarConfiguracion(config);
    validarEstructura(inicial);
    if (inicial.columnaInicio >= 5 || inicial.columnaMeta < ANCHO_NIVEL - 5) {
        throw std::invalid_argument("SA requiere inicio y meta en sus respectivas bandas extremas");
    }
    std::mt19937 rng(semilla);
    std::uniform_real_distribution<double> unidad;
    EstadisticasSA stats;
    auto actual = evaluarCandidato(inicial, instancia, config.pesos, stats, 0);
    auto mejor = actual;
    stats.costoInicial = actual.evaluacion.costoTotal;
    stats.progresoInicial = actual.evaluacion.progreso;
    stats.linealidadInicial = actual.evaluacion.linealidad;
    const double logInicial = std::log(config.temperaturaInicial);
    const double logFinal = std::log(config.temperaturaFinal);
    const int tramos = std::max(1, config.iteraciones - 1);
    for (int i = 0; i < config.iteraciones; ++i) {
        const int iteracion = i + 1;
        auto nivel = proponerVecino(actual.nivel, instancia, actual.evaluacion, actual.ruta,
                                    rng, config.probAvance, config.probAjuste);
        auto candidato = evaluarCandidato(std::move(nivel), instancia, config.pesos, stats, iteracion);
        if (mejoraLaSeleccion(candidato, mejor)) {
            mejor = candidato;
            stats.iteracionSeleccionada = iteracion;
        }
        const double fraccion = static_cast<double>(i) / tramos;
        const double temperatura = std::exp((1.0 - fraccion) * logInicial + fraccion * logFinal);
        const double delta = candidato.evaluacion.costoTotal - actual.evaluacion.costoTotal;
        if (delta <= 0 || unidad(rng) < std::exp(-delta / temperatura)) {
            actual = std::move(candidato);
            ++stats.aceptadas;
            if (delta > 0) ++stats.empeoramientosAceptados;
        }
    }
    stats.iteraciones = config.iteraciones;
    stats.segundos = std::chrono::duration<double>(Reloj::now() - inicio).count();
    return {std::move(mejor.nivel), mejor.evaluacion, std::move(mejor.ruta), stats};
}
} // namespace

ResultadoSA optimizarNivel(const Instancia& instancia, std::uint32_t semilla, const ConfiguracionSA& config) {
    const auto inicio = Reloj::now();
    const auto inicial = generarInicial(instancia, semilla);
    return buscar(instancia, inicial, semilla, config, inicio);
}

ResultadoSA optimizarNivel(const Instancia& instancia, const Nivel& inicial, std::uint32_t semilla,
                            const ConfiguracionSA& config) {
    const auto inicio = Reloj::now();
    validarInstancia(instancia);
    return buscar(instancia, inicial, semilla, config, inicio);
}
