#include "Movimiento.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr double EPS = 1e-9;
constexpr double PI = 3.14159265358979323846;
constexpr int ALTURA_MAXIMA_SALTO = 4;
constexpr double VELOCIDAD_HORIZONTAL_MAXIMA = 6.0;
constexpr int DIVISIONES_TIEMPO = 8;

enum class Tipo { Caminar, Caer, Saltar, Cualquiera };

bool soporteUtilizable(const Nivel& nivel, int col) {
    return col >= 0 && col < ANCHO_NIVEL &&
           nivel.alturaPlataforma[col] >= 0 && nivel.alturaPlataforma[col] < ALTO_NIVEL - 1 &&
           nivel.peligro[col] != Peligro::Obstaculo;
}

struct Rango {
    double min;
    double max;

    bool penetra(int fila) const {
        return min < fila + 1.0 - EPS && max + 1.0 > fila + EPS;
    }
};

// Desplazamiento monotonico con velocidad cero en los dos extremos.
// Se parametriza en celdas avanzadas, independientemente de la direccion.
struct ControlHorizontal {
    double desde;
    double hasta;
    double inicio;
    double fin;

    double posicion(double t) const {
        if (t <= inicio) return desde;
        if (t >= fin) return hasta;
        const double fase = (t - inicio) / (fin - inicio);
        return desde + (hasta - desde) * (1.0 - std::cos(PI * fase)) / 2.0;
    }

    static double faseEn(double fraccion) {
        return std::acos(1.0 - 2.0 * fraccion) / PI;
    }

    double instante(double posicion) const {
        return inicio + (fin - inicio) * faseEn((posicion - desde) / (hasta - desde));
    }
};

// Tiempo normalizado con aceleracion vertical constante de -2. Para la
// caida el apice coincide con la salida del borde y la velocidad inicial es 0.
struct ArcoVertical {
    double pico;
    double apice;
    double aterrizaje;

    double pies(double t) const {
        const double d = t - apice;
        return pico - d * d;
    }

    Rango rango(double a, double b) const {
        const double inicial = pies(a);
        const double final = pies(b);
        return {std::min(inicial, final), a <= apice && apice <= b ? pico : std::max(inicial, final)};
    }
};

struct Recorrido {
    const Nivel& nivel;
    int origen;
    int direccion;
    int distancia;

    int columna(int avance) const { return origen + direccion * avance; }

    // Barrido de los tramos apoyados, con posicion horizontal continua.
    bool tramoApoyado(double desde, double hasta, double pies, CondicionMovimiento& resultado) const {
        for (int k = 0; k <= distancia; ++k) {
            if (hasta <= k - 1 + EPS || desde >= k + 1 - EPS) continue;
            const int col = columna(k);
            const int fila = nivel.alturaPlataforma[col];
            if (fila == SIN_PLATAFORMA) continue;
            const Rango altura{pies, pies};
            if (altura.penetra(fila)) return false;
            if (!altura.penetra(fila + 1)) continue;
            if (nivel.peligro[col] == Peligro::Obstaculo) return false;
            if (nivel.peligro[col] == Peligro::Enemigo && !resultado.aplasta[col]) {
                resultado.requiere.set(col);
            }
        }
        return true;
    }

    bool tramoAereo(const ControlHorizontal& x, const ArcoVertical& y,
                    CondicionMovimiento& resultado) const {
        for (int k = 0; k <= distancia; ++k) {
            const double izquierda = k - 1.0;
            const double derecha = k + 1.0;
            if (x.hasta <= izquierda || x.desde >= derecha) continue;
            const int col = columna(k);
            const int fila = nivel.alturaPlataforma[col];
            if (fila == SIN_PLATAFORMA) continue;

            // Para una caja de ancho 1 solo se solapa esta columna
            // mientras el avance pertenece al intervalo abierto (k-1,k+1).
            const double a = izquierda < x.desde ? 0.0 : x.instante(izquierda);
            const double b = derecha > x.hasta ? y.aterrizaje
                : std::min(y.aterrizaje, x.instante(derecha));
            if (b - a <= EPS) continue;
            const Rango altura = y.rango(a, b);
            if (altura.penetra(fila)) return false;
            if (nivel.peligro[col] == Peligro::Obstaculo && altura.penetra(fila + 1)) return false;
            if (nivel.peligro[col] != Peligro::Enemigo || resultado.requiere[col]) continue;

            const double cima = fila + 2.0;
            if (y.pico > cima + EPS) {
                const double contacto = y.apice + std::sqrt(y.pico - cima);
                if (a - EPS <= contacto && contacto <= b + EPS) {
                    const double avance = x.posicion(contacto);
                    if (izquierda + EPS < avance && avance < derecha - EPS &&
                        !y.rango(a, std::max(a, contacto)).penetra(fila + 1)) {
                        resultado.aplasta.set(col);
                        continue;
                    }
                }
            }
            if (altura.penetra(fila + 1)) resultado.requiere.set(col);
        }
        return true;
    }
};

template <typename Visitar>
bool buscarControles(const Recorrido& recorrido, const ArcoVertical& y,
                     double desde, double piso, Visitar& visitar) {
    const double destino = recorrido.distancia;
    const double t = y.aterrizaje;
    const double recorridoHorizontal = destino - desde;
    const double duracionMinima = PI * recorridoHorizontal / (2.0 * VELOCIDAD_HORIZONTAL_MAXIMA);
    CondicionMovimiento salida;
    if (!recorrido.tramoApoyado(0.0, desde, y.pies(0.0), salida)) return false;
    const auto probar = [&](double inicio, double fin) {
        if (fin - inicio + EPS < duracionMinima) return false;
        const ControlHorizontal x{desde, destino, inicio, fin};
        const double llegadaHorizontal = x.posicion(t);
        if (llegadaHorizontal < destino - 1.0 - EPS) return false;

        auto opcion = salida;
        return recorrido.tramoAereo(x, y, opcion) &&
               recorrido.tramoApoyado(llegadaHorizontal, destino, piso, opcion) && visitar(opcion);
    };

    if (recorridoHorizontal == 0.0) return probar(0.0, t);
    if (probar(0.0, t)) return true;

    for (int i = 0; i < DIVISIONES_TIEMPO; ++i) {
        const double inicio = t * i / DIVISIONES_TIEMPO;
        // Completar el avance en vuelo o ya apoyado. El mismo perfil
        // horizontal sigue tras aterrizar, incluso si aun tiene velocidad.
        for (int j = i + 1; j <= DIVISIONES_TIEMPO + 2; ++j) {
            if (i == 0 && j == DIVISIONES_TIEMPO) continue;
            if (probar(inicio, t * j / DIVISIONES_TIEMPO)) return true;
        }
        // Alinea la entrada al apoyo con la altura de aterrizaje. Es el
        // control que permite llegar al borde justo en el apice maximo.
        if (recorridoHorizontal > 1.0) {
            const double fase = ControlHorizontal::faseEn(1.0 - 1.0 / recorridoHorizontal);
            if (probar(inicio, inicio + (t - inicio) / fase)) return true;
        }
    }
    // En un salto adyacente a altura maxima se alcanza la esquina sin
    // avance lateral y se empieza a caminar sobre el destino en el apice.
    return recorridoHorizontal == 1.0 && probar(t, t + duracionMinima);
}

template <typename Visitar>
bool buscarMovimientos(const Nivel& nivel, int origen, int destino, Tipo tipo, Visitar visitar) {
    if (!soporteUtilizable(nivel, origen) || !soporteUtilizable(nivel, destino)) return false;
    const int dx = destino - origen;
    const int distancia = std::abs(dx);
    if (distancia == 0 || distancia > ALCANCE_MAXIMO_MOVIMIENTO) return false;

    const Recorrido recorrido{nivel, origen, dx > 0 ? 1 : -1, distancia};
    const double pisoOrigen = nivel.alturaPlataforma[origen] + 1.0;
    const double pisoDestino = nivel.alturaPlataforma[destino] + 1.0;

    if ((tipo == Tipo::Caminar || tipo == Tipo::Cualquiera) && distancia == 1 && pisoOrigen == pisoDestino) {
        CondicionMovimiento opcion;
        if (recorrido.tramoApoyado(0.0, 1.0, pisoOrigen, opcion) && visitar(opcion)) return true;
    }
    if ((tipo == Tipo::Caer || tipo == Tipo::Cualquiera) && pisoDestino < pisoOrigen) {
        const ArcoVertical y{pisoOrigen, 0.0, std::sqrt(pisoOrigen - pisoDestino)};
        if (buscarControles(recorrido, y, 1.0, pisoDestino, visitar)) return true;
    }
    if (tipo == Tipo::Saltar || tipo == Tipo::Cualquiera) {
        for (int h = 1; h <= ALTURA_MAXIMA_SALTO; ++h) {
            const double pico = pisoOrigen + h;
            if (pico + 1.0 > ALTO_NIVEL) break;
            if (pico < pisoDestino || (pico == pisoDestino && h < ALTURA_MAXIMA_SALTO)) continue;
            const double apice = std::sqrt(h);
            const ArcoVertical y{pico, apice, apice + std::sqrt(pico - pisoDestino)};
            if (buscarControles(recorrido, y, 0.0, pisoDestino, visitar)) return true;
        }
    }
    return false;
}

bool movimientoValido(const Nivel& nivel, int origen, int destino, Tipo tipo,
                      const EnemigosAplastados& enemigos) {
    return buscarMovimientos(nivel, origen, destino, tipo, [&](const CondicionMovimiento& opcion) {
        return opcion.habilitada(enemigos);
    });
}

bool domina(const CondicionMovimiento& a, const CondicionMovimiento& b) {
    return a.habilitada(b.requiere) && (b.aplasta & ~(a.aplasta | b.requiere)).none();
}
} // namespace

bool saltoValido(const Nivel& n, int origen, int destino, const EnemigosAplastados& enemigos) {
    return movimientoValido(n, origen, destino, Tipo::Saltar, enemigos);
}

bool caidaValida(const Nivel& n, int origen, int destino, const EnemigosAplastados& enemigos) {
    return movimientoValido(n, origen, destino, Tipo::Caer, enemigos);
}

bool caminarValido(const Nivel& n, int origen, int destino, const EnemigosAplastados& enemigos) {
    return movimientoValido(n, origen, destino, Tipo::Caminar, enemigos);
}

bool movimientoDirectoValido(const Nivel& n, int origen, int destino, const EnemigosAplastados& enemigos) {
    return movimientoValido(n, origen, destino, Tipo::Cualquiera, enemigos);
}

std::vector<CondicionMovimiento> condicionesMovimiento(const Nivel& n, int origen, int destino) {
    std::vector<CondicionMovimiento> opciones;
    if (!soporteUtilizable(n, origen) || !soporteUtilizable(n, destino)) return opciones;
    const auto primero = n.peligro.begin() + std::min(origen, destino);
    const auto ultimo = n.peligro.begin() + std::max(origen, destino) + 1;
    const bool sinEnemigos = std::find(primero, ultimo, Peligro::Enemigo) == ultimo;
    buscarMovimientos(n, origen, destino, Tipo::Cualquiera, [&](const CondicionMovimiento& opcion) {
        if (std::any_of(opciones.begin(), opciones.end(), [&](const auto& otra) { return domina(otra, opcion); })) {
            return false;
        }
        opciones.erase(std::remove_if(opciones.begin(), opciones.end(), [&](const auto& otra) {
            return domina(opcion, otra);
        }), opciones.end());
        opciones.push_back(opcion);
        return sinEnemigos;
    });
    return opciones;
}
