#include "GeneradorInicial.hpp"
#include "Movimiento.hpp"
#include "SimulatedAnnealing.hpp"
#include "Vecindario.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
int pruebas = 0;

void exigir(bool condicion, const std::string& descripcion) {
    if (!condicion) throw std::runtime_error("FALLO: " + descripcion);
    ++pruebas;
    std::cout << "OK: " << descripcion << '\n';
}

bool igual(const Nivel& a, const Nivel& b) {
    return a.alturaPlataforma == b.alturaPlataforma && a.peligro == b.peligro &&
           a.columnaInicio == b.columnaInicio && a.columnaMeta == b.columnaMeta;
}

bool verificado(const Nivel& n, const ResultadoAlcanzabilidad& ruta) {
    if (!ruta.completable || ruta.camino.empty() || ruta.camino.front() != n.columnaInicio ||
        ruta.camino.back() != n.columnaMeta) return false;
    std::vector<EnemigosAplastados> estados(1);
    for (std::size_t i = 1; i < ruta.camino.size(); ++i) {
        std::vector<EnemigosAplastados> siguientes;
        const auto opciones = condicionesMovimiento(n, ruta.camino[i - 1], ruta.camino[i]);
        for (const auto& estado : estados) {
            for (const auto& opcion : opciones) {
                if (!opcion.habilitada(estado)) continue;
                const auto nuevos = estado | opcion.aplasta;
                if (std::find(siguientes.begin(), siguientes.end(), nuevos) == siguientes.end()) {
                    siguientes.push_back(nuevos);
                }
            }
        }
        estados = std::move(siguientes);
    }
    return !estados.empty();
}

void probarVecindario() {
    using Op = OperadorVecindario;
    constexpr std::array operadores = {Op::CambiarAltura, Op::TrasladarPlataforma, Op::MoverPeligro,
        Op::AgregarPlataforma, Op::QuitarPlataforma, Op::AgregarPeligro, Op::QuitarPeligro};
    const Instancia instancia{1, 75, 20, 15, ObjetivoLinealidad::Maximizar};
    const Nivel nivel = generarInicial(instancia, 42);
    const auto iniciales = contarElementos(nivel);
    std::mt19937 rng(5);
    bool movioEnemigo = false;
    bool movioObstaculo = false;
    for (auto op : operadores) {
        for (int i = 0; i < 50; ++i) {
            auto vecino = generarVecino(nivel, op, rng);
            if (!vecino || igual(*vecino, nivel)) throw std::runtime_error("operador sin cambio en tablero con espacio");
            validarEstructura(*vecino);
            if (vecino->columnaInicio != nivel.columnaInicio || vecino->columnaMeta != nivel.columnaMeta) {
                throw std::runtime_error("operador movio un extremo");
            }
            const auto c = contarElementos(*vecino);
            const int esperadoP = (op == Op::AgregarPlataforma) - (op == Op::QuitarPlataforma);
            const bool cantidadPeligros = (op == Op::AgregarPeligro || op == Op::QuitarPeligro || op == Op::QuitarPlataforma);
            if (c.plataformas != iniciales.plataformas + esperadoP ||
                (!cantidadPeligros && (c.enemigos != iniciales.enemigos || c.obstaculos != iniciales.obstaculos)) ||
                (op == Op::AgregarPeligro && c.enemigos + c.obstaculos != iniciales.enemigos + iniciales.obstaculos + 1) ||
                (op == Op::QuitarPeligro && c.enemigos + c.obstaculos != iniciales.enemigos + iniciales.obstaculos - 1)) {
                throw std::runtime_error("operador cambio conteos fuera de su alcance");
            }
            if (op == Op::MoverPeligro) {
                for (int col = 0; col < ANCHO_NIVEL; ++col) {
                    if (nivel.peligro[col] == vecino->peligro[col]) continue;
                    movioEnemigo |= nivel.peligro[col] == Peligro::Enemigo;
                    movioObstaculo |= nivel.peligro[col] == Peligro::Obstaculo;
                }
            }
        }
    }
    exigir(movioEnemigo && movioObstaculo, "350 propuestas validas y traslado de enemigos y obstaculos");

    Nivel disperso;
    disperso.alturaPlataforma[0] = disperso.alturaPlataforma[149] = 4;
    const auto ruta = verificarCompletabilidad(disperso);
    const auto puente = generarVecino(disperso, Op::AcercarRuta, rng, ruta);
    if (!puente) throw std::runtime_error("el operador dirigido no propuso un apoyo");
    validarEstructura(*puente);
    const auto extendida = verificarCompletabilidad(*puente);
    exigir(!ruta.completable && !extendida.completable && extendida.apoyosAlcanzados == 2 &&
               extendida.columnasAlcanzadas.count() == 2 && contarElementos(*puente).plataformas == 3,
           "la propuesta dirigida abre un apoyo alcanzable y puede cambiar cantidades");
    exigir(!generarVecino(disperso, Op::QuitarPlataforma, rng) && !generarVecino(disperso, Op::MoverPeligro, rng),
           "los operadores sin destino se omiten cuando el tablero solo tiene extremos");

    Nivel denso;
    denso.alturaPlataforma.fill(4);
    validarEstructura(denso);
    exigir(!generarVecino(denso, Op::TrasladarPlataforma, rng) && !generarVecino(denso, Op::AgregarPlataforma, rng),
           "un tablero lleno no genera traslados o altas de plataformas invalidas");
}

void probarEvaluacion() {
    const Instancia instancia{1, 3, 0, 0, ObjetivoLinealidad::Maximizar};
    Nivel n;
    n.alturaPlataforma[0] = n.alturaPlataforma[149] = n.alturaPlataforma[7] = 4;
    const auto a = verificarCompletabilidad(n);
    const auto ea = evaluarParaSA(n, instancia, a);
    n.alturaPlataforma[7] = SIN_PLATAFORMA;
    n.alturaPlataforma[6] = 4;
    const auto b = verificarCompletabilidad(n);
    const auto eb = evaluarParaSA(n, instancia, b);
    exigir(!a.completable && !b.completable && ea.errorCantidades == eb.errorCantidades &&
               ea.linealidad == eb.linealidad && eb.progreso > ea.progreso && eb.costoTotal < ea.costoTotal,
           "el alcance exacto distingue niveles incompletos con la misma linealidad y conteos");
}

void probarRecocido() {
    Nivel inicial;
    for (int x = 0; x < ANCHO_NIVEL; x += 5) inicial.alturaPlataforma[x] = 4;
    inicial.alturaPlataforma[149] = 4;
    const Instancia max{1, contarElementos(inicial).plataformas, 0, 0, ObjetivoLinealidad::Maximizar};
    auto min = max;
    min.objetivo = ObjetivoLinealidad::Minimizar;
    ConfiguracionSA config;
    config.iteraciones = 200;
    config.temperaturaInicial = 0.5;
    config.temperaturaFinal = 0.01;
    config.pesos.cantidades = 0.5;
    const auto plano = optimizarNivel(max, inicial, 99u, config);
    const auto irregular = optimizarNivel(min, inicial, 99u, config);
    exigir(plano.evaluacion.linealidad == 1 && irregular.evaluacion.linealidad < plano.evaluacion.linealidad,
           "SA favorece linealidades opuestas con objetivos max y min");
    exigir(igual(plano.nivel, inicial) && plano.estadisticas.iteracionSeleccionada == 0 &&
               plano.estadisticas.empeoramientosAceptados > 0 && plano.estadisticas.evaluacionesConError > 0,
           "aceptar peores candidatos y variar cantidades preserva el mejor factible");
    exigir(verificado(plano.nivel, plano.ruta) && verificado(irregular.nivel, irregular.ruta) &&
               verificarCompletabilidad(irregular.nivel).camino.size() == irregular.ruta.camino.size(),
           "las soluciones factibles conservan un camino reproducible y de longitud certificada");
    const auto otra = optimizarNivel(min, inicial, 99u, config);
    exigir(igual(irregular.nivel, otra.nivel) && irregular.ruta.camino == otra.ruta.camino &&
               irregular.evaluacion.costoTotal == otra.evaluacion.costoTotal &&
               irregular.estadisticas.aceptadas == otra.estadisticas.aceptadas &&
               irregular.estadisticas.iteracionSeleccionada == otra.estadisticas.iteracionSeleccionada &&
               irregular.estadisticas.evaluaciones == 201,
           "misma semilla y configuracion reproducen nivel, ruta y estadisticas de busqueda");

    config.iteraciones = 0;
    const auto incompleto = optimizarNivel({1, 2, 0, 0, ObjetivoLinealidad::Maximizar}, 42, config);
    exigir(!incompleto.ruta.completable && incompleto.ruta.camino.empty() &&
               incompleto.estadisticas.evaluaciones == 1 && incompleto.estadisticas.primeraFactible == -1,
           "sin factibles el resultado identifica el mejor incompleto sin camino certificado");
    config.temperaturaFinal = -1;
    bool rechazo = false;
    try {
        optimizarNivel(max, inicial, 99u, config);
    } catch (const std::invalid_argument&) {
        rechazo = true;
    }
    exigir(rechazo, "una configuracion invalida se rechaza antes de evaluar");
}
} // namespace

int main() {
    try {
        probarVecindario();
        probarEvaluacion();
        probarRecocido();
        std::cout << "Todas las " << pruebas << " verificaciones de SA superadas.\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
