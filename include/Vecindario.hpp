#pragma once

#include "Evaluador.hpp"

#include <optional>
#include <random>

enum class OperadorVecindario {
    CambiarAltura, TrasladarPlataforma, MoverPeligro,
    AgregarPlataforma, QuitarPlataforma, AgregarPeligro, QuitarPeligro, AcercarRuta
};

// Una propuesta conserva las columnas de inicio y meta del nivel de partida.
// Devuelve nullopt si el operador elegido carece de una aplicacion.
std::optional<Nivel> generarVecino(const Nivel& nivel, OperadorVecindario operador, std::mt19937& rng,
                                  const ResultadoAlcanzabilidad& ruta = {});

Nivel proponerVecino(const Nivel& nivel, const Instancia& instancia, const Evaluacion& evaluacion,
                     const ResultadoAlcanzabilidad& ruta, std::mt19937& rng,
                     double probAvance, double probAjuste);
