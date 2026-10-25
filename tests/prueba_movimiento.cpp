#include "Alcanzabilidad.hpp"
#include "Movimiento.hpp"
#include "Nivel.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

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
            exigir(saltoValido(n,10,14), "caida vertical a destino inferior");
        }
        {
            auto n = caso(10,14);
            n.alturaPlataforma[11] = 4;
            n.peligro[11] = Peligro::Enemigo;
            validarEstructura(n);
            exigir(!saltoValido(n,10,14), "enemigo inmediato bloquea despegue 1x1");
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
            exigir(!saltoValido(n,10,14), "pared lateral inmediata bloquea trayectorias");
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
            auto n = caso(10,14);
            n.peligro[14] = Peligro::Obstaculo;
            // Se usa movimiento directamente porque validarEstructura prohibe
            // peligros en inicio/meta y el test busca comprobar 'soporteSeguro'.
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
            exigir(ruta.completable && ruta.camino.front()==10 && ruta.camino.back()==22,
                   "BFS encadena saltos 10-14-18-22");
        }
        {
            auto n = caso(10,11);
            exigir(caminarValido(n,10,11), "caminar sobre apoyos adyacentes al mismo nivel");
            exigir(movimientoDirectoValido(n,10,11), "movimiento directo considera caminar");
        }
        std::cout << "Todas las " << pruebas << " pruebas geometricas superadas.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
