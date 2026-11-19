#include "Evaluador.hpp"
#include "Alcanzabilidad.hpp"
#include "GeneradorInicial.hpp"
#include "LectorInstancias.hpp"
#include "Salida.hpp"
#include "SimulatedAnnealing.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr const char* USO =
    "uso: ./plataformas [archivo.txt] [--id NUM] [--semilla NUM] [--directorio-salidas DIR]\n"
    "  --sa [--iteraciones NUM] [--temperatura-inicial NUM] [--temperatura-final NUM]\n"
    "       [--prob-avance NUM] [--prob-ajuste NUM] [--peso-cantidades NUM]\n"
    "       [--peso-linealidad NUM] [--peso-incompletabilidad NUM] [--peso-progreso NUM]";

const Instancia* buscarPorId(const std::vector<Instancia>& instancias, int id) {
    for (const auto& instancia : instancias) {
        if (instancia.id == id) return &instancia;
    }
    return nullptr;
}

int convertirEntero(const std::string& texto, const std::string& descripcion) {
    try {
        std::size_t leidos = 0;
        long long valor = std::stoll(texto, &leidos);
        if (leidos != texto.size() || valor < std::numeric_limits<int>::min() ||
            valor > std::numeric_limits<int>::max()) {
            throw std::out_of_range("fuera de rango");
        }
        return static_cast<int>(valor);
    } catch (const std::exception&) {
        throw std::runtime_error(descripcion + ": se esperaba un entero valido: " + texto);
    }
}

std::uint32_t convertirSemilla(const std::string& texto) {
    if (texto.empty() || texto.find_first_not_of("0123456789") != std::string::npos) {
        throw std::runtime_error("la semilla debe ser un entero sin signo de 32 bits");
    }
    const auto numero = std::stoull(texto);
    if (numero > std::numeric_limits<std::uint32_t>::max()) throw std::runtime_error("semilla fuera de rango");
    return static_cast<std::uint32_t>(numero);
}

bool leerOpcionSA(const std::string& argumento, const std::string& valor, ConfiguracionSA& config) {
    if (argumento == "--iteraciones") {
        config.iteraciones = convertirEntero(valor, "iteraciones");
        return true;
    }
    const std::array opciones = {
        std::pair{"--temperatura-inicial", &config.temperaturaInicial},
        std::pair{"--temperatura-final", &config.temperaturaFinal},
        std::pair{"--prob-avance", &config.probAvance},
        std::pair{"--prob-ajuste", &config.probAjuste},
        std::pair{"--peso-cantidades", &config.pesos.cantidades},
        std::pair{"--peso-linealidad", &config.pesos.linealidad},
        std::pair{"--peso-incompletabilidad", &config.pesos.incompletabilidad},
        std::pair{"--peso-progreso", &config.pesos.progreso}
    };
    for (const auto& [nombre, campo] : opciones) {
        if (argumento != nombre) continue;
        std::size_t leidos = 0;
        const double numero = std::stod(valor, &leidos);
        if (leidos != valor.size()) throw std::runtime_error("valor invalido para " + argumento);
        *campo = numero;
        return true;
    }
    return false;
}

void procesar(const Instancia& instancia, std::uint32_t semilla, const ConfiguracionSA* config,
              const std::string& directorio) {
    if (config) {
        const auto resultado = optimizarNivel(instancia, semilla, *config);
        escribirReporte(std::cout, instancia, resultado.nivel, resultado.evaluacion, resultado.ruta,
                        semilla, &resultado.estadisticas);
        std::cout << "\nReporte guardado en: "
                  << guardarReporte(instancia, resultado.nivel, resultado.evaluacion, resultado.ruta,
                                    semilla, directorio, &resultado.estadisticas) << "\n";
        return;
    }
    Nivel nivel = generarInicial(instancia, semilla);
    validarEstructura(nivel);
    Evaluacion evaluacion = evaluar(nivel, instancia);
    ResultadoAlcanzabilidad ruta = verificarCompletabilidad(nivel);
    escribirReporte(std::cout, instancia, nivel, evaluacion, ruta, semilla);
    std::cout << "\nReporte guardado en: "
              << guardarReporte(instancia, nivel, evaluacion, ruta, semilla, directorio) << "\n";
}
} // namespace

int main(int argc, char* argv[]) {
    try {
        std::string archivo = "instancias.txt";
        std::string directorio = "salidas";
        int idDirecto = -1;
        std::uint32_t semilla = 42; // pruebas reproducibles
        bool archivoAsignado = false;
        bool usarSA = false;
        bool opcionesSA = false;
        ConfiguracionSA config;

        for (int i = 1; i < argc; ++i) {
            std::string argumento = argv[i];
            if (argumento == "--help") {
                std::cout << USO << '\n';
                return 0;
            } else if (argumento == "--sa") {
                usarSA = true;
            } else if (argumento == "--id" && i + 1 < argc) {
                idDirecto = convertirEntero(argv[++i], "ID");
            } else if (argumento == "--semilla" && i + 1 < argc) {
                semilla = convertirSemilla(argv[++i]);
            } else if (argumento == "--directorio-salidas" && i + 1 < argc) {
                directorio = argv[++i];
            } else if (i + 1 < argc && leerOpcionSA(argumento, argv[i + 1], config)) {
                ++i;
                opcionesSA = true;
            } else if (!argumento.empty() && argumento[0] != '-' && !archivoAsignado) {
                archivo = argumento;
                archivoAsignado = true;
            } else {
                throw std::runtime_error(USO);
            }
        }

        if (opcionesSA && !usarSA) throw std::runtime_error("las opciones de recocido requieren --sa");
        const auto instancias = leerInstancias(archivo);
        if (idDirecto != -1) {
            const Instancia* elegida = buscarPorId(instancias, idDirecto);
            if (!elegida) throw std::runtime_error("ID solicitado no existe en el archivo");
            procesar(*elegida, semilla, usarSA ? &config : nullptr, directorio);
            return 0;
        }

        // Menú interactivo: permite probar varias instancias en una sesión.
        while (true) {
            std::cout << "\nINSTANCIAS DISPONIBLES (" << archivo << ")\n";
            std::cout << " ID | Plataformas | Enemigos | Obstaculos | Linealidad\n";
            for (const auto& instancia : instancias) {
                std::cout << " " << instancia.id << "  | " << instancia.plataformas
                          << " | " << instancia.enemigos << " | " << instancia.obstaculos
                          << " | " << nombreObjetivo(instancia.objetivo) << '\n';
            }
            std::cout << "Ingresa el ID de una instancia (0 para salir): " << std::flush;
            std::string respuesta;
            if (!std::getline(std::cin, respuesta)) break;
            int id = -1;
            try { id = convertirEntero(respuesta, "seleccion"); }
            catch (const std::exception&) { std::cout << "Entrada no valida.\n"; continue; }
            if (id == 0) break;
            const Instancia* elegida = buscarPorId(instancias, id);
            if (!elegida) {
                std::cout << "No existe el ID " << id << ".\n";
                continue;
            }
            procesar(*elegida, semilla, usarSA ? &config : nullptr, directorio);
        }
        std::cout << "Programa finalizado.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
