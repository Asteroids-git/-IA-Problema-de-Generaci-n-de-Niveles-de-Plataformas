#include "Salida.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string>

namespace {
char representarCelda(const Nivel& nivel, int x, int y) {
    const int altura = nivel.alturaPlataforma[x];
    if (altura == SIN_PLATAFORMA) return '.';
    if (y == altura) return '#';
    if (y == altura + 1) {
        if (x == nivel.columnaInicio) return 'I';
        if (x == nivel.columnaMeta) return 'F';
        if (nivel.peligro[x] == Peligro::Enemigo) return 'E';
        if (nivel.peligro[x] == Peligro::Obstaculo) return 'O';
    }
    return '.';
}
} // namespace

void escribirReporte(std::ostream& salida, const Instancia& instancia,
                    const Nivel& nivel, const Evaluacion& ev,
                    const ResultadoAlcanzabilidad& ruta, unsigned semilla) {
    salida << "INSTANCIA " << instancia.id << " | linealidad "
           << nombreObjetivo(instancia.objetivo) << " | semilla " << semilla << '\n';
    salida << "Solicitado: plataformas=" << instancia.plataformas
           << " enemigos=" << instancia.enemigos
           << " obstaculos=" << instancia.obstaculos << '\n';
    salida << "Generado:  plataformas=" << ev.conteos.plataformas
           << " enemigos=" << ev.conteos.enemigos
           << " obstaculos=" << ev.conteos.obstaculos << '\n';
    salida << std::fixed << std::setprecision(4)
           << "Error cantidades=" << ev.errorCantidades
           << " | linealidad=" << ev.linealidad
           << " | penalizacion=" << ev.penalizacionLinealidad
           << " | costo total=" << ev.costoTotal << '\n';
    salida << "Restricciones estructurales: VALIDAS\n";
    salida << "Completabilidad: " << (ruta.completable ? "COMPLETABLE" : "NO COMPLETABLE")
           << " (segun modelo discreto de saltos/BFS)\n";
    salida << "Apoyos seguros=" << ruta.apoyosSeguros
           << " | aristas dirigidas=" << ruta.movimientosValidos
           << " | apoyos alcanzados=" << ruta.apoyosAlcanzados << '\n';
    if (ruta.completable) {
        salida << "Ruta (columnas): ";
        for (std::size_t i = 0; i < ruta.camino.size(); ++i) {
            if (i) salida << " -> ";
            salida << ruta.camino[i];
        }
        salida << '\n';
    }

    salida << "Leyenda: # plataforma, E enemigo, O obstaculo, I inicio, F fin, . vacio\n\n";
    // Se imprime con y creciente hacia arriba para que el tablero se lea
    // visualmente igual que un nivel de plataformas.
    for (int y = ALTO_NIVEL - 1; y >= 0; --y) {
        salida << std::setw(2) << y << " | ";
        for (int x = 0; x < ANCHO_NIVEL; ++x) {
            salida << representarCelda(nivel, x, y);
        }
        salida << '\n';
    }
    salida << "     ";
    for (int x = 0; x < ANCHO_NIVEL; ++x) salida << x % 10;
    salida << "  <- columna modulo 10\n";
}

std::string guardarReporte(const Instancia& instancia, const Nivel& nivel,
                           const Evaluacion& ev, const ResultadoAlcanzabilidad& ruta,
                           unsigned semilla, const std::string& directorio) {
    std::filesystem::create_directories(directorio);
    const std::string rutaArchivo = directorio + "/instancia_" +
        std::to_string(instancia.id) + ".txt";
    std::ofstream archivo(rutaArchivo);
    if (!archivo) throw std::runtime_error("no se pudo crear archivo de salida: " + rutaArchivo);
    escribirReporte(archivo, instancia, nivel, ev, ruta, semilla);
    if (!archivo) throw std::runtime_error("fallo al escribir archivo de salida: " + rutaArchivo);
    return rutaArchivo;
}
