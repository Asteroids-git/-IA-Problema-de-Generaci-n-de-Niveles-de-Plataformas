#include "Evaluador.hpp"
#include "Alcanzabilidad.hpp"
#include "GeneradorInicial.hpp"
#include "LectorInstancias.hpp"
#include "Salida.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
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

void procesar(const Instancia& instancia, std::uint32_t semilla) {
    Nivel nivel = generarInicial(instancia, semilla);
    validarEstructura(nivel);
    Evaluacion evaluacion = evaluar(nivel, instancia);
    ResultadoAlcanzabilidad ruta = verificarCompletabilidad(nivel);
    escribirReporte(std::cout, instancia, nivel, evaluacion, ruta, semilla);
    std::cout << "\nReporte guardado en: "
              << guardarReporte(instancia, nivel, evaluacion, ruta, semilla) << "\n";
}
} // namespace

int main(int argc, char* argv[]) {
    try {
        std::string archivo = "instancias.txt";
        int idDirecto = -1;
        std::uint32_t semilla = 42; // pruebas reproducibles
        bool archivoAsignado = false;

        for (int i = 1; i < argc; ++i) {
            std::string argumento = argv[i];
            if (argumento == "--id" && i + 1 < argc) {
                idDirecto = convertirEntero(argv[++i], "ID");
            } else if (argumento == "--semilla" && i + 1 < argc) {
                const int numero = convertirEntero(argv[++i], "semilla");
                if (numero < 0) throw std::runtime_error("la semilla no puede ser negativa");
                semilla = static_cast<std::uint32_t>(numero);
            } else if (!argumento.empty() && argumento[0] != '-' && !archivoAsignado) {
                archivo = argumento;
                archivoAsignado = true;
            } else {
                throw std::runtime_error("uso: ./plataformas [archivo.txt] [--id NUM] [--semilla NUM]");
            }
        }

        const auto instancias = leerInstancias(archivo);
        if (idDirecto != -1) {
            const Instancia* elegida = buscarPorId(instancias, idDirecto);
            if (!elegida) throw std::runtime_error("ID solicitado no existe en el archivo");
            procesar(*elegida, semilla);
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
            procesar(*elegida, semilla);
        }
        std::cout << "Programa finalizado.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
