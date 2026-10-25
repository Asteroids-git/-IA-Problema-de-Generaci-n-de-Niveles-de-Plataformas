#include "Instancia.hpp"
#include "Nivel.hpp"

#include <stdexcept>
#include <string>

std::string nombreObjetivo(ObjetivoLinealidad objetivo) {
    return objetivo == ObjetivoLinealidad::Maximizar ? "max" : "min";
}

void validarInstancia(const Instancia& instancia) {
    if (instancia.id <= 0) {
        throw std::invalid_argument("el ID debe ser positivo");
    }
    if (instancia.plataformas < 2 || instancia.plataformas > ANCHO_NIVEL) {
        throw std::invalid_argument("la cantidad de plataformas debe estar entre 2 y 150");
    }
    if (instancia.enemigos < 0 || instancia.obstaculos < 0) {
        throw std::invalid_argument("las cantidades de enemigos y obstaculos no pueden ser negativas");
    }
    // En la versión inicial colocamos a lo sumo un peligro por plataforma
    // y reservamos plataformas sin peligros para el inicio y la meta.
    if (instancia.enemigos + instancia.obstaculos > instancia.plataformas - 2) {
        throw std::invalid_argument(
            "no caben los peligros con la regla inicial de un peligro por plataforma "
            "y plataformas libres en inicio y meta");
    }
}
