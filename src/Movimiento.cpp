#include "Movimiento.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr double EPS = 1e-9;

bool soporteSeguro(const Nivel& n, int c) {
    return c >= 0 && c < ANCHO_NIVEL &&
        n.alturaPlataforma[c] != SIN_PLATAFORMA &&
        n.peligro[c] == Peligro::Ninguno;
}

// La coordenada y de los pies durante el salto, relativa a su apoyo.
double parabola(double d, double h, double r) {
    const double t = d / r;
    return 4.0 * h * t * (1.0 - t);
}

struct Rango { double min, max; };

// Extremos EXACTOS de la parabola concava en un intervalo horizontal.
// Esto permite verificar el cuerpo completo sin simular tiempo ni
// muestrear puntos separados por una distancia arbitraria.
Rango rangoPies(double a, double b, double h, double r, double suelo) {
    const double fa = suelo + parabola(a, h, r);
    const double fb = suelo + parabola(b, h, r);
    double menor = std::min(fa, fb);
    double mayor = std::max(fa, fb);
    if (a - EPS <= r / 2.0 && r / 2.0 <= b + EPS) {
        mayor = std::max(mayor, suelo + h);
    }
    return {menor, mayor};
}

// Se verifica la trayectoria completa entre d=0 y d=D.
// La caja del personaje, centrada en x, ocupa [x-0.5,x+0.5]
// por [yPies,yPies+1]. Los bloques y peligros ocupan celdas enteras.
// Una interseccion de area positiva implica colision; tocar un borde no.
bool arcoLibre(const Nivel& n, int origen, int direccion,
               int D, double h, double r) {
    const double suelo = n.alturaPlataforma[origen] + 1.0;
    const Rango total = rangoPies(0.0, D, h, r, suelo);
    if (total.min < -EPS || total.max + 1.0 > ALTO_NIVEL + EPS) return false;

    // El cuerpo cubre un ancho de 1 y avanza como maximo 6 columnas.
    const int primera = std::max(0, std::min(origen, origen + direccion * D) - 1);
    const int ultima = std::min(ANCHO_NIVEL - 1,
                                std::max(origen, origen + direccion * D) + 1);
    for (int col = primera; col <= ultima; ++col) {
        const int fila = n.alturaPlataforma[col];
        if (fila == SIN_PLATAFORMA) continue;

        // La caja horizontal del jugador y la celda en 'col' tienen
        // interseccion positiva solo cuando d pertenece a (k-1, k+1).
        const int k = direccion * (col - origen);
        const double a = std::max(0.0, static_cast<double>(k - 1));
        const double b = std::min(static_cast<double>(D), static_cast<double>(k + 1));
        if (b - a <= EPS) continue;

        const Rango alturas = rangoPies(a, b, h, r, suelo);
        // Una funcion continua toma todos los valores entre min y max.
        // Hay penetracion en [fila,fila+1] si [pies,pies+1] tiene
        // solapamiento vertical de area positiva para algun d interior.
        const auto colisionCelda = [&](int y) {
            return alturas.min < y + 1.0 - EPS &&
                   alturas.max + 1.0 > y + EPS;
        };
        if (colisionCelda(fila)) return false;
        if (n.peligro[col] != Peligro::Ninguno && colisionCelda(fila + 1)) {
            return false;
        }
    }
    return true;
}

} // namespace

bool caminarValido(const Nivel& n, int origen, int destino) {
    if (!soporteSeguro(n, origen) || !soporteSeguro(n, destino)) return false;
    if (std::abs(destino - origen) != 1) return false;
    return n.alturaPlataforma[origen] == n.alturaPlataforma[destino];
}

bool saltoValido(const Nivel& n, int origen, int destino) {
    if (!soporteSeguro(n, origen) || !soporteSeguro(n, destino)) return false;
    const int dx = destino - origen;
    const int D = std::abs(dx);
    if (D < 1 || D > 6) return false;

    const int direccion = dx > 0 ? 1 : -1;
    const double suelo = n.alturaPlataforma[origen] + 1.0;
    const double sueloDestino = n.alturaPlataforma[destino] + 1.0;

    // Cuatro trayectorias discretizadas. Se prueba primero la mas alta.
    // Si en el centro del destino los pies estan sobre la superficie,
    // pueden detenerse horizontalmente y caer verticalmente.
    for (int h = 4; h >= 1; --h) {
        const double r = 1.5 * h;
        const double yParada = suelo + parabola(D, h, r);
        if (yParada + EPS < sueloDestino) continue;
        if (!arcoLibre(n, origen, direccion, D, h, r)) continue;

        // La caída vertical se realiza en el centro de la columna destino.
        // Como solo hay una plataforma por columna y su celda superior
        // esta libre (soporteSeguro), no hay bloques en el espacio entre
        // los pies de la parada y la superficie de destino.
        // La plataforma de destino se toca por arriba, sin penetracion.
        return true;
    }
    return false;
}

bool movimientoDirectoValido(const Nivel& n, int origen, int destino) {
    return caminarValido(n, origen, destino) || saltoValido(n, origen, destino);
}
