#include "Alcanzabilidad.hpp"
#include "Movimiento.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <queue>
#include <tuple>
#include <utility>

namespace {
struct Arista {
    int destino;
    std::vector<CondicionMovimiento> opciones;
};
using Grafo = std::array<std::vector<Arista>, ANCHO_NIVEL>;

struct BusquedaColumnas {
    std::array<int, ANCHO_NIVEL> padre;
    std::array<int, ANCHO_NIVEL> distancia;
    std::bitset<ANCHO_NIVEL> alcanzados;
};

// Cotas para evitar expandir mascaras cuando basta una ruta que solo
// depende de que el enemigo del apoyo actual haya sido eliminado.
BusquedaColumnas buscarColumnas(const Grafo& grafo, int inicio, const EnemigosAplastados* permitidos,
                                EnemigosAplastados* efectos = nullptr) {
    BusquedaColumnas busqueda{};
    busqueda.distancia.fill(-1);
    std::array<int, ANCHO_NIVEL> cola;
    cola[0] = inicio;
    busqueda.distancia[inicio] = 0;
    busqueda.alcanzados.set(inicio);
    int fin = 1;
    for (int i = 0; i < fin; ++i) {
        const int desde = cola[i];
        const auto enemigos = permitidos ? *permitidos : EnemigosAplastados{}.set(desde);
        for (const auto& arista : grafo[desde]) {
            const int destino = arista.destino;
            if (!efectos && busqueda.alcanzados[destino]) continue;
            bool habilitada = false;
            for (const auto& opcion : arista.opciones) {
                if (!opcion.habilitada(enemigos)) continue;
                habilitada = true;
                if (!efectos) break;
                *efectos |= opcion.aplasta;
            }
            if (!habilitada || busqueda.alcanzados[destino]) continue;
            busqueda.alcanzados.set(destino);
            busqueda.padre[destino] = desde;
            busqueda.distancia[destino] = busqueda.distancia[desde] + 1;
            cola[fin++] = destino;
        }
    }
    return busqueda;
}

// Reune los pisotones posibles desde columnas alcanzables bajo la cota
// anterior. Mezclar recorridos sobreestima lo que puede hacer una ruta.
BusquedaColumnas buscarCotaSuperior(const Grafo& grafo, int inicio, EnemigosAplastados& posibles) {
    for (;;) {
        auto efectos = posibles;
        const auto busqueda = buscarColumnas(grafo, inicio, &posibles, &efectos);
        if (efectos == posibles) return busqueda;
        posibles = efectos;
    }
}

bool coincide(const BusquedaColumnas& a, const BusquedaColumnas& b, int meta) {
    return a.alcanzados == b.alcanzados && a.distancia[meta] == b.distancia[meta];
}

void filtrarGrafo(Grafo& grafo, const BusquedaColumnas& cota, const EnemigosAplastados& posibles) {
    for (int col = 0; col < ANCHO_NIVEL; ++col) {
        auto& aristas = grafo[col];
        if (!cota.alcanzados[col]) {
            aristas.clear();
            continue;
        }
        for (auto& arista : aristas) {
            auto& opciones = arista.opciones;
            opciones.erase(std::remove_if(opciones.begin(), opciones.end(), [&](const auto& opcion) {
                return !opcion.habilitada(posibles);
            }), opciones.end());
        }
        aristas.erase(std::remove_if(aristas.begin(), aristas.end(), [](const auto& arista) {
            return arista.opciones.empty();
        }), aristas.end());
    }
}

// Distancia a la meta ignorando los requisitos que aun queden. Cada
// movimiento real usa una de estas aristas, asi que es una cota inferior.
std::array<int, ANCHO_NIVEL> distanciasAMeta(const Grafo& grafo, int meta) {
    std::array<int, ANCHO_NIVEL> distancias;
    distancias.fill(ANCHO_NIVEL);
    distancias[meta] = 0;
    std::array<int, ANCHO_NIVEL> cola;
    cola[0] = meta;
    int fin = 1;
    for (int i = 0; i < fin; ++i) {
        const int destino = cola[i];
        const int primero = std::max(0, destino - ALCANCE_MAXIMO_MOVIMIENTO);
        const int ultimo = std::min(ANCHO_NIVEL - 1, destino + ALCANCE_MAXIMO_MOVIMIENTO);
        for (int origen = primero; origen <= ultimo; ++origen) {
            if (distancias[origen] != ANCHO_NIVEL) continue;
            const auto& aristas = grafo[origen];
            if (std::none_of(aristas.begin(), aristas.end(), [destino](const auto& arista) {
                return arista.destino == destino;
            })) continue;
            distancias[origen] = distancias[destino] + 1;
            cola[fin++] = origen;
        }
    }
    return distancias;
}

// Un bit solo puede afectar movimientos que lo exigen. La propagacion
// inversa por el grafo potencial conserva todos los requisitos futuros,
// incluso los de aristas que todavia no estan habilitadas.
std::array<EnemigosAplastados, ANCHO_NIVEL> enemigosRelevantes(const Grafo& grafo) {
    std::array<EnemigosAplastados, ANCHO_NIVEL> relevantes{};
    for (int col = 0; col < ANCHO_NIVEL; ++col) {
        for (const auto& arista : grafo[col]) {
            for (const auto& opcion : arista.opciones) relevantes[col] |= opcion.requiere;
        }
    }
    bool cambio;
    do {
        cambio = false;
        for (int col = 0; col < ANCHO_NIVEL; ++col) {
            const auto previos = relevantes[col];
            for (const auto& arista : grafo[col]) relevantes[col] |= relevantes[arista.destino];
            cambio |= relevantes[col] != previos;
        }
    } while (cambio);
    return relevantes;
}

struct Estado {
    int columna;
    EnemigosAplastados enemigos;
    std::size_t padre;
    int pasos;
    bool activo = true;
};

bool contiene(const EnemigosAplastados& conjunto, const EnemigosAplastados& otro) {
    return (otro & ~conjunto).none();
}
} // namespace

ResultadoAlcanzabilidad verificarCompletabilidad(const Nivel& nivel, EstadisticasAlcanzabilidad* stats) {
    using Reloj = std::chrono::steady_clock;
    const auto inicioGrafo = stats ? Reloj::now() : Reloj::time_point{};
    if (stats) *stats = {};
    ResultadoAlcanzabilidad resultado;
    Grafo grafo;
    for (int i = 0; i < ANCHO_NIVEL; ++i) {
        if (nivel.alturaPlataforma[i] == SIN_PLATAFORMA ||
            nivel.alturaPlataforma[i] == ALTO_NIVEL - 1 ||
            nivel.peligro[i] == Peligro::Obstaculo) continue;
        ++resultado.apoyosSeguros;
        for (int delta = -ALCANCE_MAXIMO_MOVIMIENTO; delta <= ALCANCE_MAXIMO_MOVIMIENTO; ++delta) {
            if (delta == 0) continue;
            const int destino = i + delta;
            auto opciones = condicionesMovimiento(nivel, i, destino);
            if (opciones.empty()) continue;
            grafo[i].push_back({destino, std::move(opciones)});
            ++resultado.movimientosValidos;
        }
    }
    if (stats) stats->grafoNs = std::chrono::duration_cast<std::chrono::nanoseconds>(Reloj::now() - inicioGrafo).count();

    const int inicio = nivel.columnaInicio;
    const int destinoFinal = nivel.columnaMeta;
    EnemigosAplastados todos;
    todos.set();
    auto potencial = buscarColumnas(grafo, inicio, &todos);
    const auto garantizada = buscarColumnas(grafo, inicio, nullptr);
    EnemigosAplastados posibles;
    if (!coincide(garantizada, potencial, destinoFinal)) {
        potencial = buscarCotaSuperior(grafo, inicio, posibles);
    }
    if (coincide(garantizada, potencial, destinoFinal)) {
        resultado.columnasAlcanzadas = garantizada.alcanzados;
        resultado.apoyosAlcanzados = static_cast<int>(garantizada.alcanzados.count());
        resultado.completable = garantizada.alcanzados[destinoFinal];
        if (resultado.completable) {
            for (int col = destinoFinal; col != inicio; col = garantizada.padre[col]) {
                resultado.camino.push_back(col);
            }
            resultado.camino.push_back(inicio);
            std::reverse(resultado.camino.begin(), resultado.camino.end());
        }
        if (stats) stats->cotaCertifica = true;
        return resultado;
    }

    filtrarGrafo(grafo, potencial, posibles);
    const auto relevantes = enemigosRelevantes(grafo);
    std::vector<Estado> estados{{inicio, {}, 0, 0}};
    std::array<std::vector<std::size_t>, ANCHO_NIVEL> frontera;
    frontera[inicio].push_back(0);
    auto alcanzados = garantizada.alcanzados;
    std::size_t meta = 0;
    const bool metaPosible = potencial.alcanzados[destinoFinal];
    const auto distancias = metaPosible ? distanciasAMeta(grafo, destinoFinal) : std::array<int, ANCHO_NIVEL>{};
    // A* da prioridad a menos pasos estimados. Entre empates, si hay
    // meta posible busca mas cerca de ella. Conserva los padres por id.
    using Pendiente = std::tuple<int, int, std::size_t>;
    std::priority_queue<Pendiente, std::vector<Pendiente>, std::greater<Pendiente>> pendientes;
    pendientes.emplace(distancias[inicio], 0, 0);

    while (!pendientes.empty()) {
        const std::size_t id = std::get<2>(pendientes.top());
        pendientes.pop();
        const auto actual = estados[id];
        if (!actual.activo) continue;
        if (actual.columna == destinoFinal && meta == 0) meta = id;
        if (alcanzados == potencial.alcanzados && (meta != 0 || !metaPosible)) break;
        if (stats) ++stats->estadosExpandidos;
        const bool minimizarPasos = metaPosible && meta == 0;
        for (const auto& arista : grafo[actual.columna]) {
            const int destino = arista.destino;
            auto& candidatos = frontera[destino];
            for (const auto& opcion : arista.opciones) {
                if (!opcion.habilitada(actual.enemigos)) continue;
                const auto enemigos = (actual.enemigos | opcion.aplasta) & relevantes[destino];
                const int pasos = actual.pasos + 1;
                // Mientras se busca la ruta minima, un superconjunto
                // domina solo si llega con igual o menor costo.
                if (std::any_of(candidatos.begin(), candidatos.end(), [&](std::size_t otro) {
                    return (!minimizarPasos || estados[otro].pasos <= pasos) &&
                           contiene(estados[otro].enemigos, enemigos);
                })) continue;
                candidatos.erase(std::remove_if(candidatos.begin(), candidatos.end(), [&](std::size_t otro) {
                    auto& anterior = estados[otro];
                    if (minimizarPasos && anterior.pasos < pasos) return false;
                    if (!contiene(enemigos, anterior.enemigos)) return false;
                    anterior.activo = false;
                    return true;
                }), candidatos.end());

                const std::size_t nuevo = estados.size();
                estados.push_back({destino, enemigos, id, pasos});
                candidatos.push_back(nuevo);
                alcanzados.set(destino);
                pendientes.emplace(pasos + distancias[destino], metaPosible ? distancias[destino] : 0, nuevo);
            }
        }
    }

    if (stats) stats->estadosInsertados = estados.size();
    resultado.columnasAlcanzadas = alcanzados;
    resultado.apoyosAlcanzados = static_cast<int>(alcanzados.count());
    resultado.completable = meta != 0;
    if (resultado.completable) {
        for (std::size_t id = meta; id != 0; id = estados[id].padre) {
            resultado.camino.push_back(estados[id].columna);
        }
        resultado.camino.push_back(inicio);
        std::reverse(resultado.camino.begin(), resultado.camino.end());
    }
    return resultado;
}
