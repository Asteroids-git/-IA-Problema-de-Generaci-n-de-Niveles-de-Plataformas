#include "Vecindario.hpp"
#include "Movimiento.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
using Operador = OperadorVecindario;
constexpr std::array operadores = {Operador::CambiarAltura, Operador::TrasladarPlataforma,
    Operador::MoverPeligro, Operador::AgregarPlataforma, Operador::QuitarPlataforma,
    Operador::AgregarPeligro, Operador::QuitarPeligro};

int entero(std::mt19937& rng, int min, int max) {
    return std::uniform_int_distribution<int>(min, max)(rng);
}

template <typename Predicado>
int elegirColumna(std::mt19937& rng, Predicado apta) {
    std::array<int, ANCHO_NIVEL> opciones;
    int cantidad = 0;
    for (int col = 0; col < ANCHO_NIVEL; ++col) {
        if (apta(col)) opciones[cantidad++] = col;
    }
    return cantidad == 0 ? -1 : opciones[entero(rng, 0, cantidad - 1)];
}

bool extremo(const Nivel& n, int col) {
    return col == n.columnaInicio || col == n.columnaMeta;
}

bool vacia(const Nivel& n, int col) {
    return n.alturaPlataforma[col] == SIN_PLATAFORMA;
}

bool admitePeligro(const Nivel& n, int col) {
    return !extremo(n, col) && !vacia(n, col) && n.alturaPlataforma[col] < ALTO_NIVEL - 1 &&
           n.peligro[col] == Peligro::Ninguno;
}

std::optional<Nivel> modificar(const Nivel& n, Operador op, std::mt19937& rng,
                               const std::bitset<ANCHO_NIVEL>& protegidas = {},
                               Peligro peligro = Peligro::Ninguno, bool conservarPeligros = false) {
    Nivel v = n;
    switch (op) {
    case Operador::CambiarAltura: {
        const int col = elegirColumna(rng, [&](int x) { return !vacia(n, x); });
        if (col < 0) return {};
        const int actual = n.alturaPlataforma[col];
        const int techo = extremo(n, col) || n.peligro[col] != Peligro::Ninguno ? ALTO_NIVEL - 2 : ALTO_NIVEL - 1;
        const bool cambioLocal = entero(rng, 0, 3) != 0;
        const int min = cambioLocal ? std::max(0, actual - 3) : 0;
        const int max = cambioLocal ? std::min(techo, actual + 3) : techo;
        const int fila = entero(rng, min, max - 1);
        v.alturaPlataforma[col] = fila + (fila >= actual);
        break;
    }
    case Operador::TrasladarPlataforma: {
        const int origen = elegirColumna(rng, [&](int x) { return !vacia(n, x) && !extremo(n, x); });
        const int destino = elegirColumna(rng, [&](int x) { return vacia(n, x); });
        if (origen < 0 || destino < 0) return {};
        std::swap(v.alturaPlataforma[origen], v.alturaPlataforma[destino]);
        std::swap(v.peligro[origen], v.peligro[destino]);
        break;
    }
    case Operador::MoverPeligro: {
        const int origen = elegirColumna(rng, [&](int x) { return n.peligro[x] != Peligro::Ninguno; });
        const int destino = elegirColumna(rng, [&](int x) { return admitePeligro(n, x); });
        if (origen < 0 || destino < 0) return {};
        std::swap(v.peligro[origen], v.peligro[destino]);
        break;
    }
    case Operador::AgregarPlataforma: {
        const int col = elegirColumna(rng, [&](int x) { return vacia(n, x); });
        if (col < 0) return {};
        v.alturaPlataforma[col] = entero(rng, 0, ALTO_NIVEL - 1);
        break;
    }
    case Operador::QuitarPlataforma: {
        const int col = elegirColumna(rng, [&](int x) {
            return !vacia(n, x) && !extremo(n, x) && !protegidas[x] &&
                   (!conservarPeligros || n.peligro[x] == Peligro::Ninguno);
        });
        if (col < 0) return {};
        v.alturaPlataforma[col] = SIN_PLATAFORMA;
        v.peligro[col] = Peligro::Ninguno;
        break;
    }
    case Operador::AgregarPeligro: {
        const int col = elegirColumna(rng, [&](int x) { return admitePeligro(n, x) && !protegidas[x]; });
        if (col < 0) return {};
        if (peligro == Peligro::Ninguno) peligro = entero(rng, 0, 1) ? Peligro::Enemigo : Peligro::Obstaculo;
        v.peligro[col] = peligro;
        break;
    }
    case Operador::QuitarPeligro: {
        const int col = elegirColumna(rng, [&](int x) {
            return n.peligro[x] != Peligro::Ninguno && (peligro == Peligro::Ninguno || n.peligro[x] == peligro);
        });
        if (col < 0) return {};
        v.peligro[col] = Peligro::Ninguno;
        break;
    }
    case Operador::AcercarRuta:
        return {};
    }
    return v;
}

std::optional<Nivel> acercarRuta(const Nivel& n, const ResultadoAlcanzabilidad& ruta, std::mt19937& rng) {
    if (ruta.completable) return {};
    int origen = n.columnaInicio;
    for (int col = 0; col < ANCHO_NIVEL; ++col) {
        if (ruta.columnasAlcanzadas[col] && std::abs(n.columnaMeta - col) < std::abs(n.columnaMeta - origen)) {
            origen = col;
        }
    }
    const int distancia = std::abs(n.columnaMeta - origen);
    if (distancia == 0) return {};
    const int sentido = n.columnaMeta > origen ? 1 : -1;
    const int alcance = std::min(ALCANCE_MAXIMO_MOVIMIENTO, distancia);
    const int paso = entero(rng, 1, alcance);
    const auto preparar = [&](int destino) {
        Nivel v = n;
        if (vacia(n, destino) && entero(rng, 0, 2) != 0) {
            const int trasladada = elegirColumna(rng, [&](int x) {
                return !vacia(n, x) && !extremo(n, x) && n.peligro[x] == Peligro::Ninguno &&
                       !ruta.columnasAlcanzadas[x];
            });
            if (trasladada >= 0) v.alturaPlataforma[trasladada] = SIN_PLATAFORMA;
        }
        if (n.peligro[destino] != Peligro::Ninguno) {
            const int receptor = elegirColumna(rng, [&](int x) {
                return admitePeligro(n, x) && !ruta.columnasAlcanzadas[x];
            });
            if (receptor >= 0) v.peligro[receptor] = n.peligro[destino];
        }
        v.alturaPlataforma[destino] = n.alturaPlataforma[origen];
        v.peligro[destino] = Peligro::Ninguno;
        return v;
    };

    int destino = origen + sentido * paso;
    auto vecino = preparar(destino);
    EnemigosAplastados enemigos;
    enemigos.set(origen);
    if (paso > 1 && !movimientoDirectoValido(vecino, origen, destino, enemigos)) {
        destino = origen + sentido;
        vecino = preparar(destino);
    }
    if (vecino.alturaPlataforma == n.alturaPlataforma && vecino.peligro == n.peligro) return {};
    return vecino;
}

std::optional<Nivel> ajustarCantidades(const Nivel& n, const Instancia& instancia, const Conteos& conteos,
                                       const ResultadoAlcanzabilidad& ruta, std::mt19937& rng) {
    std::bitset<ANCHO_NIVEL> protegidas;
    for (int col : ruta.camino) protegidas.set(col);
    const std::array diferencias = {instancia.plataformas - conteos.plataformas,
        instancia.enemigos - conteos.enemigos, instancia.obstaculos - conteos.obstaculos};
    const int primero = entero(rng, 0, 2);
    for (int i = 0; i < 3; ++i) {
        const int tipo = (primero + i) % 3;
        const int delta = diferencias[tipo];
        if (delta == 0) continue;
        const auto peligro = tipo == 1 ? Peligro::Enemigo : Peligro::Obstaculo;
        const auto op = tipo == 0 ? (delta > 0 ? Operador::AgregarPlataforma : Operador::QuitarPlataforma)
                                 : (delta > 0 ? Operador::AgregarPeligro : Operador::QuitarPeligro);
        if (auto vecino = modificar(n, op, rng, protegidas, peligro, true)) return vecino;
    }
    return {};
}
} // namespace

std::optional<Nivel> generarVecino(const Nivel& nivel, OperadorVecindario op, std::mt19937& rng,
                                  const ResultadoAlcanzabilidad& ruta) {
    return op == Operador::AcercarRuta ? acercarRuta(nivel, ruta, rng) : modificar(nivel, op, rng);
}

Nivel proponerVecino(const Nivel& nivel, const Instancia& instancia, const Evaluacion& evaluacion,
                     const ResultadoAlcanzabilidad& ruta, std::mt19937& rng,
                     double probAvance, double probAjuste) {
    const double sorteo = std::uniform_real_distribution<double>{}(rng);
    if (sorteo < probAvance && !ruta.completable) {
        if (auto vecino = acercarRuta(nivel, ruta, rng)) return *vecino;
    } else if (sorteo < probAvance + probAjuste && evaluacion.errorCantidades > 0) {
        if (auto vecino = ajustarCantidades(nivel, instancia, evaluacion.conteos, ruta, rng)) return *vecino;
    }
    const int primero = entero(rng, 0, static_cast<int>(operadores.size()) - 1);
    for (std::size_t i = 0; i < operadores.size(); ++i) {
        if (auto vecino = modificar(nivel, operadores[(primero + i) % operadores.size()], rng)) return *vecino;
    }
    // Con dos apoyos validos CambiarAltura siempre tiene una aplicacion.
    return nivel;
}
