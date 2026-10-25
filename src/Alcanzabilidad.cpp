#include "Alcanzabilidad.hpp"
#include "Movimiento.hpp"

#include <algorithm>
#include <array>
#include <queue>

ResultadoAlcanzabilidad verificarCompletabilidad(const Nivel& nivel) {
    ResultadoAlcanzabilidad resultado;
    std::array<std::vector<int>, ANCHO_NIVEL> adyacencia;
    for (int i = 0; i < ANCHO_NIVEL; ++i) {
        if (nivel.alturaPlataforma[i] == SIN_PLATAFORMA ||
            nivel.peligro[i] != Peligro::Ninguno) continue;
        ++resultado.apoyosSeguros;
        // Como se parte y aterriza en centros, solo son candidatos
        // los destinos hasta seis columnas en cada sentido.
        for (int delta = -6; delta <= 6; ++delta) {
            if (delta == 0) continue;
            const int j = i + delta;
            if (movimientoDirectoValido(nivel, i, j)) {
                adyacencia[i].push_back(j);
                ++resultado.movimientosValidos;
            }
        }
    }

    // 'validarEstructura' garantiza inicio y meta existentes y seguros.
    std::queue<int> q;
    std::array<int, ANCHO_NIVEL> padre;
    padre.fill(-1);
    q.push(nivel.columnaInicio);
    padre[nivel.columnaInicio] = nivel.columnaInicio;
    while (!q.empty()) {
        const int actual = q.front(); q.pop();
        ++resultado.apoyosAlcanzados;
        for (const int vecino : adyacencia[actual]) {
            if (padre[vecino] != -1) continue;
            padre[vecino] = actual;
            q.push(vecino);
        }
    }
    resultado.completable = padre[nivel.columnaMeta] != -1;
    if (resultado.completable) {
        for (int nodo = nivel.columnaMeta; nodo != nivel.columnaInicio; nodo = padre[nodo]) {
            resultado.camino.push_back(nodo);
        }
        resultado.camino.push_back(nivel.columnaInicio);
        std::reverse(resultado.camino.begin(), resultado.camino.end());
    }
    return resultado;
}
