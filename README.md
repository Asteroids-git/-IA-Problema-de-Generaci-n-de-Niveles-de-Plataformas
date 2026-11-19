# Generación procedural de niveles de plataformas 2D

Proyecto de Inteligencia Artificial para la Entrega 2, implementado en C++17. Genera niveles de plataformas y los optimiza con recocido simulado (*Simulated Annealing*, SA). Cada instancia especifica cantidades de plataformas, enemigos y obstáculos, y un objetivo de linealidad. Un verificador exacto para la familia de movimientos implementada determina si existe una ruta entre el inicio y la meta.

## Características implementadas

- Generación inicial aleatoria con cantidades exactas y semilla configurable.
- Niveles de 150 × 16 con caminata, saltos, caídas, detección continua de colisiones y pisotones.
- Verificación de completabilidad mediante BFS, cotas de alcanzabilidad y A*, con recuperación de una ruta de longitud mínima cuando existe.
- Operadores de vecindario y SA con aceptación probabilística, enfriamiento geométrico y conservación del mejor nivel factible evaluado.
- Reportes de niveles, pruebas automatizadas, benchmarks del verificador y experimentos reproducibles con SA.

## Requisitos del entorno

- Linux, `g++` con soporte de C++17 y GNU Make.
- Bash para la suite de pruebas y los ejemplos de ejecución por lotes.
- Python 3 para ejecutar los scripts de experimentos y benchmarks.

## Compilación

Desde la raíz del repositorio, compila con `make`. El ejecutable se genera como `plataformas` y los objetos quedan en `build/`.

```sh
make
```

## Ejecución del programa

El menú interactivo permite seleccionar una instancia y salir con `0`. Sin `--sa`, el programa genera y verifica el nivel inicial. Usa `--id` para seleccionar una instancia directamente y `--sa` para activar la optimización.

```sh
./plataformas
./plataformas instancias.txt --id 3 --semilla 42
./plataformas instancias.txt --sa --id 4 --semilla 42 --iteraciones 3000
./plataformas --help
```

Para ejecutar SA en las seis instancias oficiales con la misma semilla:

```sh
for id in 1 2 3 4 5 6; do
  ./plataformas instancias.txt --sa --id "$id" --semilla 42 --iteraciones 3000
done
```

Cada reporte incluye el tablero, las cantidades, la linealidad, el costo, la completabilidad y el camino cuando existe. SA añade las iteraciones, evaluaciones, aceptaciones, progreso y tiempos. Los archivos se guardan en `salidas/` por defecto, con nombres como `instancia_3.txt` e `instancia_4_sa_42.txt`. Usa `--directorio-salidas DIR` para separar corridas con distintos parámetros.

## Formato de las instancias

El programa usa [instancias.txt](instancias.txt) por defecto. Cada línea contiene `ID PLATAFORMAS ENEMIGOS OBSTACULOS OBJETIVO`. Por ejemplo, `1 75 20 15 max` solicita 75 plataformas, 20 enemigos, 15 obstáculos y maximizar la linealidad. `min` indica minimizarla. El lector acepta líneas vacías y comentarios iniciados con `#`.

Los identificadores deben ser enteros positivos y únicos. Se admiten de 2 a 150 plataformas, cantidades de peligros no negativas y, en conjunto, como máximo `PLATAFORMAS − 2` peligros. Se reservan dos apoyos libres para el inicio y la meta.

## Parámetros configurables de SA

| Opción | Predeterminado | Función |
| --- | ---: | --- |
| `--sa` | Desactivado | Activa el recocido simulado. |
| `--semilla NUM` | `42` | Semilla de 32 bits sin signo para la generación y la búsqueda. También aplica al modo inicial. |
| `--iteraciones NUM` | `3000` | Número de vecinos evaluados. Con `0` se evalúa el candidato inicial. |
| `--temperatura-inicial NUM` | `0.08` | Temperatura al comienzo del recocido. |
| `--temperatura-final NUM` | `0.002` | Temperatura del último paso cuando se evalúan al menos dos vecinos. |
| `--prob-avance NUM` | `0.45` | Probabilidad reservada a propuestas dirigidas hacia la meta. |
| `--prob-ajuste NUM` | `0.20` | Probabilidad reservada al ajuste dirigido de cantidades. |
| `--peso-cantidades NUM` | `0.07` | Peso por unidad de error de cantidades. |
| `--peso-linealidad NUM` | `0.5` | Peso de la penalización de linealidad. |
| `--peso-incompletabilidad NUM` | `1.5` | Peso de la penalización por meta inalcanzable. |
| `--peso-progreso NUM` | `3.0` | Peso del progreso pendiente. |

El presupuesto es un entero no negativo. Las temperaturas deben ser finitas y cumplir `T_inicial ≥ T_final > 0`. Las dos probabilidades deben ser no negativas y sumar como máximo 1. Los pesos deben ser finitos, no negativos y producir un costo finito. Las opciones de recocido requieren `--sa`. [Implementación de la interfaz](src/main.cpp)

## Representación del nivel

- La cuadrícula tiene columnas de `0` a `149` y filas de `0` a `15`, con origen abajo a la izquierda. Cada columna admite como máximo un bloque de plataforma. `alturaPlataforma[x]` guarda su fila o `-1` si la columna está vacía.
- Cada plataforma admite un peligro como máximo, situado en la celda inmediatamente superior. Las plataformas de la fila `15` forman parte de la geometría. Los apoyos transitables y las plataformas con peligro requieren espacio dentro del tablero encima del bloque.
- El inicio se selecciona en las columnas `0..4` y la meta en `145..149`, con una separación mínima de 141 columnas. Ambos se apoyan en plataformas libres de peligros. El vecindario conserva sus columnas y puede cambiar sus alturas.

## Modelo de movimiento y alcanzabilidad

El personaje ocupa una caja de **1 × 1 celda**. Los movimientos parten y terminan en el centro de un apoyo. La caminata conecta apoyos contiguos a la misma altura. Los saltos ensayan elevaciones de 1 a 4 celdas desde los pies al despegue, con un alcance horizontal máximo de 6 columnas. En una caída de borde el personaje pierde el solapamiento con el apoyo y comienza a descender con velocidad vertical inicial cero. El límite horizontal incluye el paso de salida.

En el aire, la aceleración vertical es constante en tiempo normalizado. El avance horizontal usa un perfil cosenoidal de velocidad continua, con inicio y duración ajustables y rapidez máxima de 6 celdas por unidad de ese tiempo. Se ensayan intervalos en octavos del tiempo de vuelo y perfiles alineados con el borde. El barrido continuo de la caja comprueba plataformas, peligros y límites a lo largo de cada trayectoria candidata. [Modelo de movimiento](src/Movimiento.cpp)

El contacto entre bordes está permitido mientras la caja permanezca fuera del interior de las celdas ocupadas. Se admite aterrizar en el ápice de un salto de altura máxima sobre una plataforma sólida si la trayectoria está libre y se alcanza el borde o la superficie. El personaje puede terminar de centrarse ya apoyado. Un pisotón exige contacto desde arriba, velocidad vertical descendente y solapamiento horizontal positivo. Puede ocurrir sobre un enemigo intermedio y el descenso continúa sin rebote. El enemigo aplastado deja de bloquear durante esa ruta.

El verificador precalcula los requisitos y los efectos de los movimientos. BFS por columna obtiene cotas que pueden certificar qué apoyos se alcanzan y la distancia mínima a la meta. Si las cotas discrepan, A* explora estados que incluyen la columna y los enemigos eliminados relevantes. La poda por dominancia descarta estados cubiertos por otros. El resultado indica si hay un camino y, cuando existe, devuelve una ruta con el menor número de movimientos para la familia implementada. Una arista potencial puede exigir que se haya aplastado antes a un enemigo. [Verificador de alcanzabilidad](src/Alcanzabilidad.cpp)

## Función de evaluación y operadores de vecindario

La linealidad `L` es `1 − variación normalizada`, donde la variación es la suma de las diferencias absolutas de altura entre plataformas consecutivas ordenadas por columna, dividida por `15 × número de transiciones`. La penalización es `1 − L` para `max` y `L` para `min`.

SA minimiza la suma ponderada del error de cantidades, la penalización de linealidad, un indicador de incompletabilidad y el progreso pendiente. El error suma las diferencias absolutas de plataformas, enemigos y obstáculos. En un nivel incompleto, el progreso combina el avance hacia la meta desde la columna alcanzable más cercana (90 %) con la fracción de columnas del tablero alcanzadas (10 %). Un camino certificado fija el progreso en 1. [Evaluación](src/Evaluador.cpp)

El vecindario cambia alturas, traslada plataformas con su peligro, mueve peligros y agrega o quita elementos. Quitar una plataforma también quita su peligro. Las propuestas dirigidas intentan extender el alcance hacia la meta o acercar las cantidades a lo solicitado. La estructura se valida en cada evaluación. [Operadores](src/Vecindario.cpp)

El recocido acepta mejoras y empates. Un empeoramiento `Δ` se acepta con probabilidad `exp(−Δ/T)`. La temperatura decrece geométricamente. El algoritmo conserva el mejor nivel factible evaluado, incluso si la búsqueda acepta después un vecino peor. Si se agota el presupuesto sin hallar una ruta, devuelve el mejor candidato incompleto identificado como tal. [Implementación de SA](src/SimulatedAnnealing.cpp)

## Pruebas automatizadas

```sh
make clean && make && make test
```

| Suite | Cobertura |
| --- | --- |
| [Movimiento y alcanzabilidad](tests/prueba_movimiento.cpp) | 63 verificaciones. Incluyen saltos, caídas, colisiones, pisotones, 4.350 pares contrastados con una referencia geométrica y 150 niveles comparados con BFS exhaustiva. |
| [Generador](tests/prueba_generador.cpp) | 32 verificaciones de conteos, semillas, estructura y casos límite. |
| [Simulated Annealing](tests/prueba_sa.cpp) | 11 verificaciones de operadores, evaluación `max` y `min`, reproducibilidad, caminos certificados y conservación del mejor factible. |
| [Integración por consola](tests/pruebas.sh) | Procesamiento de las 6 instancias oficiales, menú, entrada inválida y opciones del modo SA. |

## Experimentos y resultados principales

Se registraron **90 ejecuciones** con las 6 instancias oficiales, 5 semillas (`0`, `7`, `42`, `2026`, `12345`) y 3 configuraciones de temperatura. Cada ejecución usó 3.000 iteraciones y evaluó 3.001 candidatos contando el nivel inicial. Las 90 ejecuciones produjeron **niveles completables con error de cantidades igual a cero**.

| Configuración | Temperatura inicial | Temperatura final | Datos |
| --- | ---: | ---: | --- |
| Base | 0,08 | 0,002 | [base.csv](experimentos/resultados/base.csv) |
| Fría | 0,04 | 0,001 | [fria.csv](experimentos/resultados/fria.csv) |
| Caliente | 0,16 | 0,004 | [caliente.csv](experimentos/resultados/caliente.csv) |

Linealidad final media de las cinco semillas por instancia:

| Instancia | Objetivo | Base | Fría | Caliente |
| ---: | :---: | ---: | ---: | ---: |
| 1 | `max` | 0,801081 | **0,831712** | 0,773153 |
| 2 | `max` | 0,858231 | **0,899320** | 0,828299 |
| 3 | `min` | 0,578378 | **0,498559** | 0,597297 |
| 4 | `min` | 0,564714 | **0,522559** | 0,574276 |
| 5 | `min` | 0,587211 | **0,475374** | 0,634014 |
| 6 | `min` | 0,583243 | **0,524685** | 0,610270 |

La configuración **fría** obtuvo la mejor linealidad promedio en las seis instancias según su objetivo. La conclusión se limita a las configuraciones y semillas evaluadas. La optimalidad global queda abierta.

El siguiente comando repite la configuración fría para las 30 combinaciones de instancia y semilla, con las salidas en `build/`:

```sh
python3 experimentos/correr_sa.py --semillas 0 7 42 2026 12345 --iteraciones 3000 \
  --temperatura-inicial 0.04 --temperatura-final 0.001 \
  --salida build/experimentos-sa/fria.csv --reportes build/experimentos-sa/fria
```

Consulta el [procedimiento experimental](experimentos/README.md) y los [benchmarks del verificador](benchmarks/README.md). El objetivo `make benchmark` ejecuta estos últimos.

## Estructura de carpetas

| Ruta | Contenido |
| --- | --- |
| [`include/`](include/) | Interfaces de los módulos en C++. |
| [`src/`](src/) | Generador, movimiento, verificador, evaluación, vecindario, SA y consola. |
| [`tests/`](tests/) | Pruebas automatizadas en C++ y Bash. |
| [`experimentos/`](experimentos/) | Script, documentación y resultados de SA. |
| [`benchmarks/`](benchmarks/) | Herramientas y mediciones del verificador. |
| `build/`, `salidas/` | Archivos de compilación y reportes generados. |

## Limitaciones conocidas

- La verificación es exacta para una **familia discreta de trayectorias** con alturas y tiempos predefinidos y avance horizontal monótono. La detección de colisiones es continua dentro de cada trayectoria.
- Los enemigos permanecen estáticos hasta ser aplastados. Su eliminación se registra para la ruta evaluada.
- La cantidad de estados de alcanzabilidad puede crecer exponencialmente con las combinaciones de enemigos eliminados relevantes, incluso con cotas y podas.
- SA depende de la semilla, la configuración y el presupuesto de iteraciones. La reproducibilidad presupone la misma implementación de la biblioteca estándar. Algunas corridas pueden agotar su presupuesto con un nivel incompleto.
