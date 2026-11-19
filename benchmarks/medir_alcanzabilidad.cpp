#include "Alcanzabilidad.hpp"
#include "GeneradorInicial.hpp"
#include "LectorInstancias.hpp"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>

#ifdef MEDIR_HEAP
namespace {
struct alignas(std::max_align_t) Bloque {
    std::size_t contabilizado;
};
bool medir = false;
std::size_t vivo = 0;
std::size_t pico = 0;
}

void* operator new(std::size_t cantidad) {
    if (cantidad > static_cast<std::size_t>(-1) - sizeof(Bloque)) throw std::bad_alloc();
    auto* memoria = std::malloc(sizeof(Bloque) + (cantidad ? cantidad : 1));
    if (!memoria) throw std::bad_alloc();
    auto* bloque = ::new (memoria) Bloque{medir ? cantidad : 0};
    vivo += bloque->contabilizado;
    if (vivo > pico) pico = vivo;
    return bloque + 1;
}
void operator delete(void* memoria) noexcept {
    if (!memoria) return;
    auto* bloque = static_cast<Bloque*>(memoria) - 1;
    vivo -= bloque->contabilizado;
    std::free(bloque);
}
void operator delete(void* memoria, std::size_t) noexcept { ::operator delete(memoria); }
void* operator new[](std::size_t cantidad) { return ::operator new(cantidad); }
void operator delete[](void* memoria) noexcept { ::operator delete(memoria); }
void operator delete[](void* memoria, std::size_t) noexcept { ::operator delete(memoria); }
#endif

namespace {
Nivel construir(const std::string& escenario, int enemigos) {
    Nivel nivel;
    if (escenario == "denso") {
        nivel.alturaPlataforma.fill(4);
        for (int col = 2; col < nivel.columnaMeta; col += 2) nivel.peligro[col] = Peligro::Enemigo;
    } else if (escenario == "bloqueo") {
        // Las rutas que suponen eliminado el enemigo de fila 13 existen.
        // Su cara superior queda a la altura maxima de los pies en el tablero.
        for (int col = 0; col <= 69; ++col) nivel.alturaPlataforma[col] = 4;
        nivel.alturaPlataforma[70] = 7;
        nivel.alturaPlataforma[72] = 10;
        for (int col = 74; col < ANCHO_NIVEL; ++col) nivel.alturaPlataforma[col] = 13;
        nivel.peligro[82] = Peligro::Enemigo;
        for (int i = 0; i < enemigos; ++i) nivel.peligro[2 + 3 * i] = Peligro::Enemigo;
    } else if (escenario == "memoria") {
        // Extiende la regresion 4 -> 8 -> 7 -> 11 con una aproximacion
        // larga y enemigos opcionales. El pisoton final sigue siendo necesario.
        nivel.columnaMeta = 111;
        for (int col = 0; col <= 104; ++col) nivel.alturaPlataforma[col] = 2;
        nivel.alturaPlataforma[107] = nivel.alturaPlataforma[108] = 1;
        nivel.peligro[108] = Peligro::Enemigo;
        nivel.alturaPlataforma[109] = 6;
        nivel.peligro[109] = Peligro::Obstaculo;
        nivel.alturaPlataforma[111] = 4;
        for (int i = 0; i < enemigos; ++i) nivel.peligro[2 + 3 * i] = Peligro::Enemigo;
    } else {
        throw std::runtime_error("Escenario desconocido " + escenario);
    }
    validarEstructura(nivel);
    return nivel;
}

void medirNivel(const Nivel& nivel, const std::string& escenario, int id, int semilla, int repeticiones) {
    for (int rep = 0; rep < repeticiones; ++rep) {
        EstadisticasAlcanzabilidad stats;
#ifdef MEDIR_HEAP
        pico = 0;
        medir = true;
#endif
        const auto inicio = std::chrono::steady_clock::now();
        const auto ruta = verificarCompletabilidad(nivel, &stats);
        const auto tiempo = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - inicio).count();
        std::size_t heap = 0;
#ifdef MEDIR_HEAP
        medir = false;
        heap = pico;
#endif
        const auto pasos = ruta.completable ? static_cast<int>(ruta.camino.size()) - 1 : -1;
        std::cout << escenario << ',' << id << ',' << semilla << ',' << rep << ','
                  << tiempo << ',' << stats.grafoNs << ',' << stats.estadosExpandidos << ','
                  << stats.estadosInsertados << ',' << stats.cotaCertifica << ',' << heap << ','
                  << ruta.completable << ',' << pasos << ',' << ruta.apoyosAlcanzados << ','
                  << ruta.movimientosValidos << '\n';
    }
}
}

int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc > 4) throw std::runtime_error(
            "Uso: medir_alcanzabilidad oficiales [repeticiones] [semillas] | denso|bloqueo|memoria [repeticiones] [enemigos]");
        const std::string escenario = argv[1];
        const int repeticiones = argc > 2 ? std::stoi(argv[2]) : 5;
        const int cantidad = argc > 3 ? std::stoi(argv[3]) : escenario == "oficiales" ? 64 : 12;
        if (repeticiones < 1 || cantidad < 0 || (escenario == "oficiales" ? cantidad < 1 : cantidad > 20)) {
            throw std::runtime_error("Cantidad o repeticiones fuera de rango");
        }
        std::cout << "escenario,id,semilla,rep,total_ns,grafo_ns,expandidos,insertados,certificado,heap_bytes,"
                     "completable,pasos,apoyos_alcanzados,aristas_potenciales\n";
        if (escenario == "oficiales") {
            const auto instancias = leerInstancias("instancias.txt");
            verificarCompletabilidad(generarInicial(instancias.front(), 42));
            for (int semilla = 0; semilla < cantidad; ++semilla) {
                for (const auto& instancia : instancias) {
                    medirNivel(generarInicial(instancia, semilla), escenario, instancia.id, semilla, repeticiones);
                }
            }
        } else {
            medirNivel(construir(escenario, cantidad), escenario, escenario == "denso" ? 74 : cantidad,
                       0, repeticiones);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
