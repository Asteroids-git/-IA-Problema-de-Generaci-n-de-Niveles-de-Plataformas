#pragma once

#include "Instancia.hpp"

#include <string>
#include <vector>

// Formato: id plataformas enemigos obstaculos max|min
// Una instancia por línea. Ignora líneas vacías y comentarios '#'.
std::vector<Instancia> leerInstancias(const std::string& ruta);
