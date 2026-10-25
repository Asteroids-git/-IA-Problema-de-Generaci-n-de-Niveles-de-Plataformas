#include "Evaluador.hpp"

#include <cmath>

Evaluacion evaluar(const Nivel& nivel, const Instancia& instancia) {
    Evaluacion resultado;
    resultado.conteos = contarElementos(nivel);
    resultado.errorCantidades =
        std::abs(instancia.plataformas - resultado.conteos.plataformas) +
        std::abs(instancia.enemigos - resultado.conteos.enemigos) +
        std::abs(instancia.obstaculos - resultado.conteos.obstaculos);

    // Propuesta de linealidad: 1 - variación absoluta normalizada de
    // alturas de plataformas consecutivas, ordenadas por columna.
    int anterior = SIN_PLATAFORMA;
    int transiciones = 0;
    int variacion = 0;
    for (int x = 0; x < ANCHO_NIVEL; ++x) {
        if (nivel.alturaPlataforma[x] == SIN_PLATAFORMA) continue;
        const int altura = nivel.alturaPlataforma[x];
        if (anterior != SIN_PLATAFORMA) {
            variacion += std::abs(altura - anterior);
            ++transiciones;
        }
        anterior = altura;
    }
    resultado.linealidad = transiciones == 0
        ? 1.0
        : 1.0 - static_cast<double>(variacion) / (transiciones * (ALTO_NIVEL - 1));

    resultado.penalizacionLinealidad =
        instancia.objetivo == ObjetivoLinealidad::Maximizar
            ? 1.0 - resultado.linealidad
            : resultado.linealidad;

    // Peso de propuesta: si se mantiene 0 < lambda < 1, corregir
    // una unidad de error de cantidades siempre es preferible a cualquier
    // mejora de linealidad (ambas cantidades están normalizadas).
    constexpr double LAMBDA = 0.5;
    resultado.costoTotal = resultado.errorCantidades +
                           LAMBDA * resultado.penalizacionLinealidad;
    return resultado;
}
