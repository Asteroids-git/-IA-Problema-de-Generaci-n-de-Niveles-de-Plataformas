#pragma once

#include "Evaluador.hpp"
#include "Alcanzabilidad.hpp"
#include "Instancia.hpp"
#include "Nivel.hpp"

#include <iosfwd>
#include <string>

// Imprime y guarda la misma representación para presentar los resultados.
void escribirReporte(std::ostream& salida, const Instancia& instancia,
                    const Nivel& nivel, const Evaluacion& evaluacion,
                    const ResultadoAlcanzabilidad& ruta, unsigned semilla);
std::string guardarReporte(const Instancia& instancia, const Nivel& nivel,
                           const Evaluacion& evaluacion, const ResultadoAlcanzabilidad& ruta, unsigned semilla,
                           const std::string& directorio = "salidas");
