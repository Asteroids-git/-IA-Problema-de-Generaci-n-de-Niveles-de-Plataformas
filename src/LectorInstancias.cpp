#include "LectorInstancias.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<Instancia> leerInstancias(const std::string& ruta) {
    std::ifstream archivo(ruta);
    if (!archivo) {
        throw std::runtime_error("no se pudo abrir el archivo de instancias: " + ruta);
    }

    std::vector<Instancia> instancias;
    std::set<int> ids;
    std::string linea;
    int numeroLinea = 0;

    while (std::getline(archivo, linea)) {
        ++numeroLinea;
        linea = linea.substr(0, linea.find('#')); // comentarios al final de línea
        std::istringstream entrada(linea);
        std::string primero;
        if (!(entrada >> primero)) continue; // línea vacía

        Instancia instancia;
        std::string sentido;
        std::string extra;
        try {
            std::size_t leidos = 0;
            instancia.id = std::stoi(primero, &leidos);
            if (leidos != primero.size()) throw std::invalid_argument("ID no numerico");
        } catch (const std::exception&) {
            throw std::runtime_error("linea " + std::to_string(numeroLinea) + ": ID invalido");
        }
        if (!(entrada >> instancia.plataformas >> instancia.enemigos >> instancia.obstaculos >> sentido) ||
            (entrada >> extra)) {
            throw std::runtime_error("linea " + std::to_string(numeroLinea) +
                ": se esperaban 5 campos: id plataformas enemigos obstaculos max|min");
        }
        if (sentido == "max") instancia.objetivo = ObjetivoLinealidad::Maximizar;
        else if (sentido == "min") instancia.objetivo = ObjetivoLinealidad::Minimizar;
        else throw std::runtime_error("linea " + std::to_string(numeroLinea) +
            ": linealidad debe ser max o min");

        try {
            validarInstancia(instancia);
        } catch (const std::exception& error) {
            throw std::runtime_error("linea " + std::to_string(numeroLinea) + ": " + error.what());
        }
        if (!ids.insert(instancia.id).second) {
            throw std::runtime_error("linea " + std::to_string(numeroLinea) +
                ": ID repetido: " + std::to_string(instancia.id));
        }
        instancias.push_back(instancia);
    }
    if (instancias.empty()) throw std::runtime_error("el archivo no contiene instancias");
    return instancias;
}
