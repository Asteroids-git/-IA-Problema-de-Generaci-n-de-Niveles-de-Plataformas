#include "Alcanzabilidad.hpp"
#include "Movimiento.hpp"
#include "Nivel.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
int pruebas = 0;
void exigir(bool condicion, const std::string& descripcion) {
    ++pruebas;
    if (!condicion) throw std::runtime_error("FALLO prueba: " + descripcion);
    std::cout << "OK: " << descripcion << '\n';
}
Nivel caso(int origen, int destino, int alturaOrigen=4, int alturaDestino=4) {
    Nivel n;
    n.columnaInicio = origen;
    n.columnaMeta = destino;
    n.alturaPlataforma[origen] = alturaOrigen;
    n.alturaPlataforma[destino] = alturaDestino;
    validarEstructura(n);
    return n;
}

// Referencia por eventos en lugar de extremos por columna. Entre dos cruces
// de cuadricula la caja ocupa las mismas celdas. En los cruces verticales
// descendentes puede cambiar el estado del enemigo bajo los pies.
constexpr double MARGEN_REFERENCIA = 1e-8;
constexpr double SEMIPI = 1.57079632679489661923;

bool cajaLibre(const Nivel& n, double x, double y, const EnemigosAplastados& enemigos) {
    const int primeraColumna = std::floor(x + MARGEN_REFERENCIA);
    const int ultimaColumna = std::ceil(x + 1.0 - MARGEN_REFERENCIA) - 1;
    const int primeraFila = std::floor(y + MARGEN_REFERENCIA);
    const int ultimaFila = std::ceil(y + 1.0 - MARGEN_REFERENCIA) - 1;
    if (primeraColumna < 0 || ultimaColumna >= ANCHO_NIVEL ||
        primeraFila < 0 || ultimaFila >= ALTO_NIVEL) return false;
    for (int col = primeraColumna; col <= ultimaColumna; ++col) {
        const int fila = n.alturaPlataforma[col];
        if (fila == SIN_PLATAFORMA) continue;
        if (primeraFila <= fila && fila <= ultimaFila) return false;
        const bool peligro = n.peligro[col] == Peligro::Obstaculo ||
            (n.peligro[col] == Peligro::Enemigo && !enemigos[col]);
        if (peligro && primeraFila <= fila + 1 && fila + 1 <= ultimaFila) return false;
    }
    return true;
}

// Las posiciones horizontales son las del borde izquierdo del personaje.
bool tramoApoyadoReferencia(const Nivel& n, double x0, double x1, double y,
                            const EnemigosAplastados& enemigos) {
    double anterior = std::min(x0, x1);
    const double fin = std::max(x0, x1);
    for (int x = std::floor(anterior) + 1; x < fin; ++x) {
        if (!cajaLibre(n, (anterior + x) / 2, y, enemigos)) return false;
        anterior = x;
    }
    return cajaLibre(n, (anterior + fin) / 2, y, enemigos);
}

bool trayectoriaReferencia(const Nivel& n, int origen, int destino, int h, double vuelo,
                           double inicio, double fin, EnemigosAplastados enemigos) {
    const int sentido = destino > origen ? 1 : -1;
    const int salida = origen + (h == 0 ? sentido : 0);
    const int avance = destino - salida;
    if (SEMIPI * std::abs(avance) > 6.0 * (fin - inicio) + MARGEN_REFERENCIA) return false;
    const double arriba = n.alturaPlataforma[origen] + 1.0;
    const double abajo = n.alturaPlataforma[destino] + 1.0;
    if (!tramoApoyadoReferencia(n, origen, salida, arriba, enemigos)) return false;
    const auto posicion = [&](double t) {
        const double s = std::sin(SEMIPI * std::clamp((t - inicio) / (fin - inicio), 0.0, 1.0));
        return salida + avance * s * s;
    };
    const double llegada = posicion(vuelo);
    if (std::abs(llegada - destino) > 1.0 + MARGEN_REFERENCIA) return false;

    const double mitadVelocidad = std::sqrt(static_cast<double>(h));
    const auto pies = [&](double t) { return arriba + t * (2 * mitadVelocidad - t); };
    std::vector<double> eventos{0.0, vuelo};
    const auto registrar = [&](double t) {
        if (0.0 < t && t < vuelo) eventos.push_back(t);
    };
    registrar(inicio);
    registrar(fin);
    for (int k = 1; k < std::abs(avance); ++k) {
        const double fase = std::asin(std::sqrt(static_cast<double>(k) / std::abs(avance))) / SEMIPI;
        registrar(inicio + (fin - inicio) * fase);
    }
    for (int y = 0; y <= ALTO_NIVEL && y <= arriba + h; ++y) {
        const double delta = std::sqrt(arriba + h - y);
        registrar(mitadVelocidad - delta);
        registrar(mitadVelocidad + delta);
    }
    std::sort(eventos.begin(), eventos.end());
    for (std::size_t i = 1; i < eventos.size(); ++i) {
        const double t = eventos[i];
        if (t - eventos[i - 1] < MARGEN_REFERENCIA) continue;
        const double medio = (eventos[i - 1] + t) / 2.0;
        if (!cajaLibre(n, posicion(medio), pies(medio), enemigos)) return false;

        if (t <= mitadVelocidad + MARGEN_REFERENCIA) continue;
        const double x = posicion(t);
        const double y = pies(t);
        for (int col = std::floor(x); col <= std::ceil(x); ++col) {
            if (std::abs(col - x) >= 1.0 - MARGEN_REFERENCIA) continue;
            if (n.peligro[col] == Peligro::Enemigo &&
                std::abs(y - n.alturaPlataforma[col] - 2.0) < MARGEN_REFERENCIA) enemigos.set(col);
        }
    }
    return tramoApoyadoReferencia(n, llegada, destino, abajo, enemigos);
}

bool movimientoReferencia(const Nivel& n, int origen, int destino, bool porCaida,
                          const EnemigosAplastados& enemigos) {
    const int distancia = std::abs(destino - origen);
    if (distancia < 1 || distancia > 6) return false;
    for (const int col : {origen, destino}) {
        const int fila = n.alturaPlataforma[col];
        if (fila < 0 || fila >= 15 || n.peligro[col] == Peligro::Obstaculo) return false;
    }
    const int desnivel = n.alturaPlataforma[destino] - n.alturaPlataforma[origen];
    if (porCaida && desnivel >= 0) return false;
    const int avance = distancia - (porCaida ? 1 : 0);
    const int hMin = porCaida ? 0 : 1;
    const int hMax = porCaida ? 0 : std::min(4, 14 - n.alturaPlataforma[origen]);
    for (int h = hMin; h <= hMax; ++h) {
        if (h < desnivel || (h == desnivel && h != 4)) continue;
        const double vuelo = std::sqrt(static_cast<double>(h)) + std::sqrt(static_cast<double>(h - desnivel));
        const auto simular = [&](double inicio, double fin) {
            return trayectoriaReferencia(n, origen, destino, h, vuelo, inicio, fin, enemigos);
        };
        if (avance == 0) return simular(0.0, vuelo);
        for (int a = 0; a < 8; ++a) {
            for (int b = a + 1; b <= 10; ++b) {
                if (simular(a * vuelo / 8, b * vuelo / 8)) return true;
            }
            if (avance > 1) {
                const double fase = std::asin(std::sqrt(1.0 - 1.0 / avance)) / SEMIPI;
                const double inicio = a * vuelo / 8;
                if (simular(inicio, inicio + (vuelo - inicio) / fase)) return true;
            }
        }
        if (avance == 1 && simular(vuelo, vuelo + SEMIPI / 6)) return true;
    }
    return false;
}

void contrastarConReferencia() {
    int casos = 0;
    const auto contrastar = [&](const Nivel& n, int origen, int destino, const EnemigosAplastados& enemigos) {
        ++casos;
        const bool salto = movimientoReferencia(n, origen, destino, false, enemigos);
        const bool caida = movimientoReferencia(n, origen, destino, true, enemigos);
        const bool caminar = caminarValido(n, origen, destino, enemigos);
        const auto opciones = condicionesMovimiento(n, origen, destino);
        const bool hayOpcion = std::any_of(opciones.begin(), opciones.end(), [&](const auto& opcion) {
            return opcion.habilitada(enemigos);
        });
        if (saltoValido(n, origen, destino, enemigos) == salto &&
            caidaValida(n, origen, destino, enemigos) == caida &&
            hayOpcion == (salto || caida || caminar) &&
            movimientoDirectoValido(n, origen, destino, enemigos) == hayOpcion) return;
        std::string detalle;
        for (int col = std::min(origen, destino); col <= std::max(origen, destino); ++col) {
            detalle += " [" + std::to_string(col) + ", " + std::to_string(n.alturaPlataforma[col]) +
                       ", " + std::to_string(static_cast<int>(n.peligro[col])) + "]";
        }
        throw std::runtime_error("Diferencia con la referencia de " + std::to_string(origen) +
                                 " a " + std::to_string(destino) + ", salto esperado " +
                                 std::to_string(salto) + ", caida esperada " + std::to_string(caida) +
                                 ". Columna, fila, peligro:" + detalle);
    };

    const auto base = caso(40,100);
    for (int filaOrigen = 0; filaOrigen < 15; ++filaOrigen) {
        for (int filaDestino = 0; filaDestino < 15; ++filaDestino) {
            for (int distancia = 1; distancia <= 7; ++distancia) {
                auto n = base;
                n.alturaPlataforma[0] = filaOrigen;
                n.alturaPlataforma[distancia] = filaDestino;
                contrastar(n, 0, distancia, {});
                n.peligro[distancia] = Peligro::Enemigo;
                contrastar(n, 0, distancia, {});
            }
        }
    }

    std::mt19937 azar(42);
    std::uniform_int_distribution<int> fila(SIN_PLATAFORMA, ALTO_NIVEL - 1);
    std::uniform_int_distribution<int> peligro(0, 2);
    std::uniform_int_distribution<int> distancia(1, 6);
    for (int i = 0; i < 600; ++i) {
        auto n = base;
        const int origen = i % 2 == 0 ? 0 : ANCHO_NIVEL - 1;
        const int direccion = origen == 0 ? 1 : -1;
        for (int d = 0; d <= 6; ++d) {
            const int col = origen + direccion * d;
            const int y = fila(azar);
            n.alturaPlataforma[col] = y;
            if (0 <= y && y < ALTO_NIVEL - 1) n.peligro[col] = static_cast<Peligro>(peligro(azar));
        }
        const int destino = origen + direccion * distancia(azar);
        validarEstructura(n);
        EnemigosAplastados enemigos;
        for (int col = std::min(origen, destino); col <= std::max(origen, destino); ++col) {
            if (n.peligro[col] == Peligro::Enemigo && azar() % 2 == 0) enemigos.set(col);
        }
        contrastar(n, origen, destino, {});
        contrastar(n, destino, origen, enemigos);
    }
    exigir(true, std::to_string(casos) + " pares contrastados para salto, caida y grafo de movimientos");
}

// Busca con mascaras completas y repite estados que solo difieren en un
// enemigo, incluso si ningun movimiento posterior lo necesita.
std::pair<int, int> bfsExhaustivo(const Nivel& n) {
    struct Estado {
        int col;
        EnemigosAplastados enemigos;
        int pasos;
    };
    struct Arista {
        int destino;
        std::vector<CondicionMovimiento> opciones;
    };
    std::array<std::vector<Arista>, ANCHO_NIVEL> grafo;
    for (int a = 0; a < ANCHO_NIVEL; ++a) {
        if (n.alturaPlataforma[a] == SIN_PLATAFORMA) continue;
        for (int b = std::max(0, a - 6); b <= std::min(ANCHO_NIVEL - 1, a + 6); ++b) {
            auto opciones = condicionesMovimiento(n, a, b);
            if (!opciones.empty()) grafo[a].push_back({b, std::move(opciones)});
        }
    }

    std::array<std::vector<EnemigosAplastados>, ANCHO_NIVEL> vistos;
    vistos[n.columnaInicio].push_back({});
    std::vector<Estado> cola{{n.columnaInicio, {}, 0}};
    int pasosMeta = -1;
    for (std::size_t i = 0; i < cola.size(); ++i) {
        const auto actual = cola[i];
        if (actual.col == n.columnaMeta && pasosMeta == -1) pasosMeta = actual.pasos;
        for (const auto& arista : grafo[actual.col]) {
            auto& mascaras = vistos[arista.destino];
            for (const auto& opcion : arista.opciones) {
                if (!opcion.habilitada(actual.enemigos)) continue;
                const auto enemigos = actual.enemigos | opcion.aplasta;
                if (std::find(mascaras.begin(), mascaras.end(), enemigos) != mascaras.end()) continue;
                mascaras.push_back(enemigos);
                cola.push_back({arista.destino, enemigos, actual.pasos + 1});
            }
        }
    }
    const int apoyos = std::count_if(vistos.begin(), vistos.end(), [](const auto& mascaras) {
        return !mascaras.empty();
    });
    return {pasosMeta, apoyos};
}

bool reproducirCamino(const Nivel& n, const std::vector<int>& camino) {
    if (camino.empty() || camino.front() != n.columnaInicio || camino.back() != n.columnaMeta) return false;
    std::vector<EnemigosAplastados> estados(1);
    for (std::size_t i = 1; i < camino.size(); ++i) {
        std::vector<EnemigosAplastados> siguientes;
        const auto opciones = condicionesMovimiento(n, camino[i - 1], camino[i]);
        for (const auto& enemigos : estados) {
            for (const auto& opcion : opciones) {
                if (!opcion.habilitada(enemigos)) continue;
                const auto nuevo = enemigos | opcion.aplasta;
                if (std::find(siguientes.begin(), siguientes.end(), nuevo) == siguientes.end()) {
                    siguientes.push_back(nuevo);
                }
            }
        }
        estados = std::move(siguientes);
    }
    return !estados.empty();
}

void contrastarBfs() {
    std::mt19937 azar(88);
    constexpr int niveles = 150;
    for (int i = 0; i < niveles; ++i) {
        const int alturaInicio = azar() % 15;
        const int alturaMeta = azar() % 15;
        auto n = caso(2, 10, alturaInicio, alturaMeta);
        for (int col = 0; col < 12; ++col) {
            if (col == n.columnaInicio || col == n.columnaMeta || azar() % 5 == 0) continue;
            n.alturaPlataforma[col] = azar() % 16;
            if (n.alturaPlataforma[col] == 15) continue;
            if (azar() % 3 == 0) n.peligro[col] = Peligro::Enemigo;
            else if (azar() % 4 == 0) n.peligro[col] = Peligro::Obstaculo;
        }
        validarEstructura(n);
        const auto [pasos, apoyos] = bfsExhaustivo(n);
        const auto ruta = verificarCompletabilidad(n);
        const int pasosRuta = ruta.completable ? static_cast<int>(ruta.camino.size()) - 1 : -1;
        if (pasosRuta != pasos || ruta.apoyosAlcanzados != apoyos ||
            (ruta.completable && !reproducirCamino(n, ruta.camino))) {
            throw std::runtime_error("Diferencia con la BFS exhaustiva en nivel " + std::to_string(i));
        }
    }
    exigir(true, std::to_string(niveles) + " niveles contrastados con BFS exhaustiva y rutas reproducibles");
}
} // namespace

int main() {
    try {
        {
            auto n = caso(10,14);
            exigir(saltoValido(n,10,14), "salto sin obstaculos hasta 4 columnas");
            exigir(saltoValido(n,14,10), "salto simetrico a la izquierda");
            exigir(verificarCompletabilidad(n).completable, "BFS en dos apoyos conectados");
        }
        {
            auto n = caso(10,17);
            exigir(!saltoValido(n,10,17), "imposible saltar 7 columnas");
            exigir(!verificarCompletabilidad(n).completable, "BFS rechaza distancia excesiva");
        }
        {
            auto n = caso(10,13,4,7);
            exigir(saltoValido(n,10,13), "plataforma elevada 3 celdas alcanzable");
        }
        {
            auto n = caso(10,13,4,9);
            exigir(!saltoValido(n,10,13), "plataforma 5 celdas mas alta inalcanzable");
        }
        {
            auto n = caso(10,14,4,1);
            exigir(saltoValido(n,10,14), "salto con aterrizaje en un destino inferior");
        }
        {
            auto n = caso(10,14);
            n.alturaPlataforma[11] = 4;
            n.peligro[11] = Peligro::Enemigo;
            validarEstructura(n);
            exigir(saltoValido(n,10,14), "el control lateral permite despegar antes de avanzar sobre el enemigo");
        }
        {
            auto n = caso(10,14);
            n.alturaPlataforma[13] = 9; // techo sobre ruta: salto alto choca
            validarEstructura(n);
            exigir(saltoValido(n,10,14), "salto bajo evita techo que bloquea salto alto");
        }
        {
            auto n = caso(10,14);
            n.alturaPlataforma[11] = 5; // bloque elevado adyacente: pared lateral
            validarEstructura(n);
            exigir(saltoValido(n,10,14), "el salto puede subir antes de cruzar el bloque lateral inmediato");
        }
        {
            auto n = caso(10,14);
            n.alturaPlataforma[13] = 4;
            n.alturaPlataforma[15] = 4;
            n.peligro[13] = Peligro::Enemigo;
            n.peligro[15] = Peligro::Enemigo;
            validarEstructura(n);
            exigir(saltoValido(n,10,14),
                   "caida centrada cabe entre dos enemigos vecinos");
        }
        {
            auto n = caso(10,15);
            n.alturaPlataforma[14] = 4;
            n.peligro[14] = Peligro::Obstaculo;
            validarEstructura(n);
            exigir(!saltoValido(n,10,14), "destino ocupado por obstaculo no es apoyo seguro");
        }
        {
            auto n = caso(10,14);
            n.alturaPlataforma[100] = 4;
            validarEstructura(n);
            auto ruta = verificarCompletabilidad(n);
            exigir(ruta.completable, "plataforma aislada NO impide completabilidad");
            exigir(ruta.apoyosSeguros == 3 && ruta.apoyosAlcanzados == 2,
                   "BFS distingue soportes seguros de alcanzados");
            exigir(ruta.camino.size()==2 && ruta.camino[0]==10 && ruta.camino[1]==14,
                   "BFS recupera ruta de columnas");
        }
        {
            auto n = caso(10,22);
            n.alturaPlataforma[14] = 4;
            n.alturaPlataforma[18] = 4;
            validarEstructura(n);
            auto ruta = verificarCompletabilidad(n);
            exigir(ruta.completable && ruta.camino == std::vector<int>{10, 14, 18, 22},
                   "BFS encadena saltos 10-14-18-22");
        }
        {
            auto n = caso(10,11);
            exigir(caminarValido(n,10,11), "caminar sobre apoyos adyacentes al mismo nivel");
            exigir(movimientoDirectoValido(n,10,11), "movimiento directo considera caminar");
        }
        {
            auto n = caso(10,11,2,5);
            exigir(saltoValido(n,10,11),
                   "salto alto y corto supera 3 filas en una columna");
        }
        {
            auto n = caso(10,17,2,2);
            n.alturaPlataforma[11] = 2;
            n.peligro[11] = Peligro::Enemigo;
            validarEstructura(n);
            exigir(!caminarValido(n,10,11), "caminar contra un enemigo es una colision lateral");
            exigir(saltoValido(n,10,11),
                   "aterrizaje sobre enemigo aplastable a una columna");
            const auto ruta = verificarCompletabilidad(n);
            exigir(ruta.completable && ruta.camino == std::vector<int>{10, 11, 17},
                   "BFS exige pisoton y despegue desde el apoyo intermedio");

            n.peligro[17] = Peligro::Enemigo;
            bool rechazoMeta = false;
            try {
                validarEstructura(n);
            } catch (const std::runtime_error&) {
                rechazoMeta = true;
            }
            exigir(rechazoMeta, "la validacion estructural rechaza un enemigo en la meta");
        }
        {
            auto n = caso(10,11,4,4);
            n.alturaPlataforma[30] = 15;
            validarEstructura(n);
            exigir(verificarCompletabilidad(n).apoyosSeguros == 2,
                   "geometria en fila 15 no es apoyo transitable");
        }
        {
            auto n = caso(10,12,4,4);
            n.alturaPlataforma[11] = 2;
            n.peligro[11] = Peligro::Obstaculo;
            validarEstructura(n);
            exigir(!caminarValido(n,10,11),
                   "un obstaculo no se atraviesa caminando");
        }
        {
            auto n = caso(0,6,0,0);
            exigir(saltoValido(n,0,6) && saltoValido(n,6,0),
                   "seis columnas al ras del piso en ambos sentidos");
            n.alturaPlataforma[6] = 1;
            exigir(saltoValido(n,0,6) && saltoValido(n,6,0),
                   "a seis columnas el control horizontal permite subir o bajar una fila");
            exigir(!saltoValido(n,-1,0) && !saltoValido(n,0,ANCHO_NIVEL) &&
                       !saltoValido(n,0,0) && !saltoValido(n,0,1),
                   "indices fuera del tablero, mismo origen y destino sin apoyo");
        }
        {
            auto n = caso(10,16);
            n.alturaPlataforma[13] = 5;
            n.peligro[13] = Peligro::Obstaculo;
            validarEstructura(n);
            exigir(saltoValido(n,10,16), "se puede pasar por encima de un obstaculo intermedio");

            n.alturaPlataforma[10] = n.alturaPlataforma[16] = 13;
            n.alturaPlataforma[11] = n.alturaPlataforma[15] = 13;
            for (const int col : {11, 15}) {
                n.peligro[col] = Peligro::Obstaculo;
                validarEstructura(n);
                exigir(!saltoValido(n,10,16),
                       col == 11 ? "el cuerpo choca al despegar antes de llegar al obstaculo"
                                 : "el cuerpo choca al final despues de pasar el obstaculo");
                n.peligro[col] = Peligro::Ninguno;
            }
        }
        {
            auto n = caso(143,149,13,13);
            exigir(saltoValido(n,143,149) && saltoValido(n,149,143),
                   "arco de seis columnas con la cabeza tangente al limite superior");

            n.alturaPlataforma[146] = 15;
            validarEstructura(n);
            exigir(!saltoValido(n,143,149) && !saltoValido(n,149,143),
                   "la fila 15 bloquea la cabeza durante el arco en ambos sentidos");
            exigir(!saltoValido(n,143,146) && !saltoValido(n,146,149),
                   "la fila 15 sirve de geometria y no de origen ni destino");
            const auto ruta = verificarCompletabilidad(n);
            exigir(!ruta.completable && ruta.camino.empty() && ruta.apoyosSeguros == 2 &&
                       ruta.apoyosAlcanzados == 1,
                   "BFS pierde la conexion por el techo y excluye su bloque como nodo");
        }
        {
            auto n = caso(10,16,13,13);
            n.alturaPlataforma[11] = 13;
            n.peligro[11] = Peligro::Enemigo;
            validarEstructura(n);
            EnemigosAplastados enemigos;
            enemigos.set(11);
            exigir(!saltoValido(n,10,16) && !verificarCompletabilidad(n).completable &&
                       saltoValido(n,10,16,enemigos),
                   "a la altura del techo el enemigo lateral impide el cruce mientras vive");
        }
        {
            auto n = caso(148,149,10,14);
            exigir(saltoValido(n,148,149), "salto adyacente de cuatro filas justo bajo el techo");
            exigir(!saltoValido(n,149,148), "desde la fila 14 no cabe un salto positivo");
            exigir(verificarCompletabilidad(n).camino == std::vector<int>{148, 149},
                   "BFS usa el salto alto y corto en el borde derecho");
            auto espejo = caso(1,0,2,6);
            exigir(saltoValido(espejo,1,0), "salto adyacente de cuatro filas hacia la izquierda");
            espejo.alturaPlataforma[0] = 7;
            exigir(!saltoValido(espejo,1,0), "un salto adyacente de cinco filas rebasa el limite");
        }
        {
            auto n = caso(10,17,2,5);
            n.alturaPlataforma[11] = 5;
            n.peligro[11] = Peligro::Enemigo;
            validarEstructura(n);
            exigir(!saltoValido(n,10,11) && !verificarCompletabilidad(n).completable,
                   "tocar un enemigo justo en el apice no es un pisoton descendente");

            n.alturaPlataforma[11] = 4;
            validarEstructura(n);
            exigir(saltoValido(n,10,11), "el salto puede pisar al mismo enemigo cuando queda bajo el apice");
            const auto ruta = verificarCompletabilidad(n);
            exigir(ruta.completable && ruta.camino == std::vector<int>{10, 11, 17},
                   "BFS recupera la ruta que requiere un pisoton durante el descenso");
            n.peligro[11] = Peligro::Obstaculo;
            validarEstructura(n);
            exigir(!saltoValido(n,10,11) && !verificarCompletabilidad(n).completable,
                   "un obstaculo en el mismo apoyo no permite el aterrizaje");
        }
        {
            auto n = caso(10,16,4,8);
            exigir(saltoValido(n,10,16), "aterrizaje en el apice maximo a seis columnas");
            n.alturaPlataforma[15] = 9;
            validarEstructura(n);
            exigir(!saltoValido(n,10,16), "un techo en la aproximacion impide aterrizar en el apice");
        }
        {
            auto n = caso(0,3,14,10);
            exigir(!saltoValido(n,0,3) && caidaValida(n,0,3) && !caidaValida(n,3,0),
                   "desde la fila 14 se puede caer hacia un apoyo inferior sin saltar");
            exigir(verificarCompletabilidad(n).camino == std::vector<int>{0, 3},
                   "BFS usa la caida como un movimiento directo");
            n.alturaPlataforma[1] = 15;
            validarEstructura(n);
            exigir(!caidaValida(n,0,3) && !verificarCompletabilidad(n).completable,
                   "el tramo apoyado de salida tambien comprueba colisiones");
            n.alturaPlataforma[1] = 13;
            n.peligro[1] = Peligro::Obstaculo;
            validarEstructura(n);
            exigir(!caidaValida(n,0,3) && !verificarCompletabilidad(n).completable,
                   "el obstaculo admite tangencia al caminar pero bloquea el inicio de la caida");
        }
        {
            auto n = caso(148,142,14,10);
            n.alturaPlataforma[147] = 12;
            n.peligro[147] = Peligro::Enemigo;
            validarEstructura(n);
            const auto opciones = condicionesMovimiento(n,148,147);
            exigir(caidaValida(n,148,147) && !saltoValido(n,148,147) &&
                       opciones.size() == 1 && opciones.front().requiere.none() && opciones.front().aplasta[147],
                   "la caida hacia la izquierda pisa a un enemigo con velocidad descendente");
            n.alturaPlataforma[147] = 13;
            validarEstructura(n);
            exigir(!caidaValida(n,148,147), "el contacto al inicio de la caida carece de velocidad descendente");
        }
        {
            auto n = caso(10,15);
            n.alturaPlataforma[14] = 4;
            n.peligro[14] = Peligro::Enemigo;
            validarEstructura(n);
            const auto opciones = condicionesMovimiento(n,10,15);
            exigir(opciones.size() == 1 && opciones.front().requiere.none() && opciones.front().aplasta[14],
                   "el salto pisa al enemigo intermedio mientras sigue avanzando");
            exigir(verificarCompletabilidad(n).camino == std::vector<int>{10, 15},
                   "BFS conserva el pisoton que ocurre dentro de un movimiento directo");
            EnemigosAplastados enemigos;
            enemigos.set(14);
            exigir(!caminarValido(n,14,15) && caminarValido(n,14,15,enemigos),
                   "salir de un apoyo con enemigo requiere que ya haya sido aplastado");
        }
        {
            auto n = caso(4,11,2,4);
            n.alturaPlataforma[7] = n.alturaPlataforma[8] = 1;
            n.peligro[8] = Peligro::Enemigo;
            n.alturaPlataforma[9] = 6;
            n.peligro[9] = Peligro::Obstaculo;
            validarEstructura(n);
            EnemigosAplastados enemigos;
            enemigos.set(8);
            exigir(movimientoDirectoValido(n,4,7) && !movimientoDirectoValido(n,7,11) &&
                       movimientoDirectoValido(n,7,11,enemigos),
                   "llegar primero a la columna 7 no habilita la salida mientras vive el enemigo");
            const auto ruta = verificarCompletabilidad(n);
            exigir(ruta.completable && ruta.camino == std::vector<int>{4, 8, 7, 11} &&
                       reproducirCamino(n, ruta.camino),
                   "BFS vuelve a la columna 7 con el enemigo eliminado en el recorrido previo");
            exigir(n.peligro[8] == Peligro::Enemigo && !movimientoDirectoValido(n,7,11),
                   "la eliminacion pertenece a la ruta y el nivel comienza con el enemigo vivo");
        }
        {
            auto n = caso(0,149);
            n.alturaPlataforma.fill(4);
            for (int col = 2; col < n.columnaMeta; col += 2) n.peligro[col] = Peligro::Enemigo;
            validarEstructura(n);
            const auto ruta = verificarCompletabilidad(n);
            exigir(ruta.completable && ruta.apoyosAlcanzados == ANCHO_NIVEL && ruta.camino.size() == 26,
                   "un tablero de 150 apoyos y 74 enemigos conserva la ruta de 25 movimientos");
        }
        contrastarConReferencia();
        contrastarBfs();
        std::cout << "Todas las " << pruebas << " pruebas geometricas superadas.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
