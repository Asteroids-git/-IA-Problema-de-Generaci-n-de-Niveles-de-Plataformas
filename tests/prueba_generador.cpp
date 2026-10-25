#include "Evaluador.hpp"
#include "GeneradorInicial.hpp"
#include "Instancia.hpp"
#include "Nivel.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int pruebas = 0;

void exigir(bool condicion, const std::string& descripcion) {
    ++pruebas;
    if (!condicion) throw std::runtime_error("FALLO: " + descripcion);
    std::cout << "OK: " << descripcion << '\n';
}

bool igual(const Nivel& a, const Nivel& b) {
    return a.alturaPlataforma == b.alturaPlataforma &&
           a.peligro == b.peligro &&
           a.columnaInicio == b.columnaInicio &&
           a.columnaMeta == b.columnaMeta;
}

void validarConteos(const Nivel& nivel, const Instancia& instancia) {
    validarEstructura(nivel);
    const auto c = contarElementos(nivel);
    exigir(c.plataformas == instancia.plataformas &&
           c.enemigos == instancia.enemigos &&
           c.obstaculos == instancia.obstaculos,
           "conteos correctos para instancia " + std::to_string(instancia.id));
}
}

int main() {
    try {
        // Las seis instancias oficiales deben funcionar con distintas semillas.
        const Instancia oficiales[] = {
            {1, 75, 20, 15, ObjetivoLinealidad::Maximizar},
            {2, 50, 20, 15, ObjetivoLinealidad::Maximizar},
            {3, 75, 20, 15, ObjetivoLinealidad::Minimizar},
            {4, 100, 20, 15, ObjetivoLinealidad::Minimizar},
            {5, 50, 20, 15, ObjetivoLinealidad::Minimizar},
            {6, 75, 15, 35, ObjetivoLinealidad::Minimizar}
        };
        for (const auto& instancia : oficiales) {
            for (std::uint32_t semilla : {0u, 1u, 42u, 12345u}) {
                validarConteos(generarInicial(instancia, semilla), instancia);
            }
        }

        const Instancia instancia{7, 75, 20, 15, ObjetivoLinealidad::Maximizar};
        const Nivel nivelA = generarInicial(instancia, 42);
        const Nivel nivelB = generarInicial(instancia, 42);
        exigir(igual(nivelA, nivelB), "misma instancia y semilla producen el mismo nivel");

        const Nivel nivelC = generarInicial(instancia, 43);
        exigir(!igual(nivelA, nivelC), "cambiar la semilla cambia la disposicion");

        Instancia instanciaMin = instancia;
        instanciaMin.objetivo = ObjetivoLinealidad::Minimizar;
        const Nivel nivelMin = generarInicial(instanciaMin, 42);
        exigir(igual(nivelA, nivelMin),
               "la solucion inicial NO depende de max/min linealidad");

        exigir(evaluar(nivelA, instancia).errorCantidades == 0,
               "evaluacion del generador conserva cantidades exactas");

        // Casos limite para no dejar constantes ligadas a las instancias.
        validarConteos(generarInicial({8, 2, 0, 0, ObjetivoLinealidad::Maximizar}, 7),
                       {8, 2, 0, 0, ObjetivoLinealidad::Maximizar});
        validarConteos(generarInicial({9, 150, 70, 78, ObjetivoLinealidad::Minimizar}, 7),
                       {9, 150, 70, 78, ObjetivoLinealidad::Minimizar});

        std::cout << "Todas las " << pruebas << " verificaciones del generador superadas.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
