# Proyecto de IA: Generación de niveles de plataformas — Estado de Avance (v3)

Código **C++17 para Linux**. Esta versión conserva el lector de instancias,
menú, evaluador, verificación de saltos, colisiones y BFS de la versión 2.
**Novedad v3:** el generador inicial es aleatorio y no adapta las posiciones ni
las alturas al objetivo `max`/`min`.

## Compilar y ejecutar

```bash
make
./plataformas
```

El menú permite seleccionar múltiples instancias del archivo `instancias.txt`.
Una instancia por línea: `id plataformas enemigos obstaculos max|min`.
También se puede ejecutar directamente:

```bash
./plataformas instancias.txt --id 3 --semilla 42
make test
make clean
```

`make test` ejecuta los seis casos oficiales, pruebas de menú/lectura,
**19 verificaciones de movimiento/colisión/BFS** y **30 verificaciones del
generador aleatorio** (incluye seis instancias × cuatro semillas, casos extremos,
reproducibilidad y ausencia de dependencia del objetivo). `make clean` borra
binario y objetos, pero mantiene `salidas/`.

## Representación

- Tablero de **150 columnas por 16 filas**; origen `(0,0)` abajo a la izquierda.
- `alturaPlataforma[c]`: `-1` si está vacía o fila entera de una plataforma.
  La representación permite como máximo una plataforma por columna.
- `peligro[c]`: ninguno, enemigo u obstáculo, ubicado una celda sobre el bloque.
- Inicio y meta son plataformas seguras. Por ahora, el generador usa las
  columnas extremas 0 y 149; **esta es una decisión provisional propia**.
- Enemigos y obstáculos son peligros estáticos, ambos impiden entrar en su celda.

## Movimiento y completabilidad (modelo simplificado adoptado)

El personaje es un **rectángulo de 1×1 celda**. Se evalúan apoyos y paradas
horizontales en **centros de columnas**. El movimiento no simula velocidades,
aceleraciones, ni habilidad del jugador.

Se consideran **cuatro saltos** con alturas máximas `h = 4, 3, 2, 1` y alcance a
la misma altura `r(h) = 1.5*h`. La altura de los pies a distancia horizontal `d`
respecto del despegue es:

```text
pies(d) = sueloOrigen + 4*h*(d/r)*(1-d/r)
```

Una trayectoria candidata llega al **centro de la columna destino** con los pies
al menos a la altura del apoyo de destino; desde ahí puede detener horizontalmente
el movimiento y **caer verticalmente** hasta la superficie. El desplazamiento
horizontal total no debe superar **6 celdas**. También puede bajar siguiendo la
parábola hasta el centro de la columna de destino. Se admite el contacto exacto
entre bordes, pero **no** que el personaje penetre plataformas o peligros.

### Detección de colisiones sin simulación de tiempos

`Movimiento.cpp` calcula mínimos y máximos de la parábola en cada intervalo de
solapamiento horizontal entre la caja 1×1 y la celda potencialmente ocupada.
Como la parábola es continua, esto detecta una intersección de área positiva
**para cada una de las trayectorias discretizadas**, sin muestrear pasos de tiempo.
Las trayectorias se descartan si el cuerpo sale verticalmente del tablero.

Cuando se detiene en el centro de una columna segura, la caída vertical es
libre porque hay como máximo una plataforma en dicha columna y no existe peligro
encima de ella. El descenso termina en el primer apoyo de esa columna.

`Alcanzabilidad.cpp` construye conexiones entre apoyos seguros a lo sumo a seis
columnas de distancia. Existe una arista dirigida si se puede caminar entre dos
apoyos adyacentes iguales, o si algún salto es válido. **BFS** busca una ruta
desde la plataforma inicial hasta la meta. No exige que todos los apoyos sean
alcanzables; las plataformas aisladas son posibles.

> **Alcance de la garantía:** el programa verifica la jugabilidad únicamente
> bajo **estas reglas simplificadas**. La elección de alturas 1–4, la relación
> `r(h)=1.5h`, los centros de columnas, el tamaño 1×1 y la caída vertical son
> decisiones del modelo y deben justificarse. No prueban jugabilidad en un motor
> de videojuego real ni abarcan todos los saltos continuos imaginables.

## Evaluación y generación inicial

Se mantienen los cálculos previos:

- `error = |P-Psol| + |E-Esol| + |O-Osol|`.
- `L = 1 - sum(|h_siguiente-h_anterior|)/(15*(Psol-1))`, para al menos dos apoyos.
- `costo = error + 0.5*(1-L)` si la instancia maximiza linealidad;
  `costo = error + 0.5*L` si la minimiza.

La medida de linealidad y el peso de 0.5 son propuestas del proyecto.

### Generación inicial aleatoria (v3)

`GeneradorInicial.cpp` utiliza `std::mt19937` con la semilla recibida:

1. Reserva las columnas **0 y 149** para inicio y meta (hipótesis provisional
   del proyecto, pendiente de confirmar con el ayudante).
2. Elige **P − 2 columnas interiores sin reemplazo**, barajando las 148 candidatas.
3. Asigna a cada plataforma una **altura aleatoria entre 0 y 14**, inclusive.
4. Baraja los apoyos interiores y sitúa **E enemigos y O obstáculos** sin
   superposición y dejando inicio y meta seguros.
5. Valida las restricciones estructurales. **No** garantiza completabilidad.

La inicialización **no consulta el objetivo de linealidad**: dos instancias con
iguales cantidades y la misma semilla, pero objetivos `max` y `min`, producen el
mismo nivel. El objetivo solamente influye en `Evaluador.cpp`.

Para reproducir un resultado utiliza, por ejemplo:

```bash
./plataformas instancias.txt --id 1 --semilla 42
```

Cambiar la semilla permite obtener otras distribuciones. Con la misma semilla e
instancia los resultados son idénticos en una misma compilación/plataforma.

**Limitación importante:** se generan niveles estructuralmente correctos, pero
sin construir primero una ruta segura. Para la semilla 42, los seis casos
oficiales se reportan como `NO COMPLETABLE` bajo el validador del proyecto.
Esto **no implica un fallo del BFS** ni que se haya implementado simulated
annealing. La completabilidad se informa por separado en el reporte.

En particular, para las instancias **2 y 5**, 50 plataformas menos 35 peligros
implican solamente 15 apoyos seguros; la instancia **6** deja 25. Con inicio
fijo en columna 0 y meta en 149, el avance de un salto limitado a seis columnas
requiere **al menos 26 apoyos seguros**. Esas tres instancias son incompatibles
con esos supuestos simultáneamente. Se debe consultar si procede otra elección
de inicio/meta o una interpretación distinta de peligros, **sin modificar
arbitrariamente las reglas**.

Los reportes `salidas/instancia_ID.txt` indican:

- Cantidades, linealidad y costo.
- Estado de restricciones estructurales.
- `COMPLETABLE` o `NO COMPLETABLE` **según este modelo**.
- Número de apoyos seguros, conexiones dirigidas y apoyos alcanzados.
- Camino de columnas cuando existe.

## Estructura de archivos

| Archivo | Responsabilidad |
|---|---|
| `include/Nivel.hpp`, `src/Nivel.cpp` | Representación y estructura |
| `include/Instancia.hpp`, `src/Instancia.cpp` | Datos y validación de instancia |
| `include/LectorInstancias.hpp`, `src/LectorInstancias.cpp` | Lector de texto |
| `include/GeneradorInicial.hpp`, `src/GeneradorInicial.cpp` | Solución inicial |
| `include/Evaluador.hpp`, `src/Evaluador.cpp` | Costo objetivo |
| **`include/Movimiento.hpp`, `src/Movimiento.cpp`** | **Saltos, caminata, colisiones** |
| **`include/Alcanzabilidad.hpp`, `src/Alcanzabilidad.cpp`** | **Grafo implícito y BFS** |
| `include/Salida.hpp`, `src/Salida.cpp` | Reportes y tablero |
| `src/main.cpp` | Menú y coordinación |
| `tests/prueba_movimiento.cpp` | 19 pruebas de movimiento, colisiones y BFS |
| `tests/prueba_generador.cpp` | 30 verificaciones del generador aleatorio |
| `tests/pruebas.sh` | Seis casos oficiales y lectura |

## Trabajo próximo

1. Confirmar con el ayudante los supuestos que impiden completar ciertas
   instancias (extremos, comportamiento de peligros).
2. Diseñar movimientos de vecindario y política de factibilidad para SA.
3. Implementar **simulated annealing** y realizar experimentos reproducibles.
4. Registrar la retroalimentación del Estado de Avance para la Entrega 2.

## Uso de IA

ChatGPT ayudó a definir simplificaciones, redactar documentación, proponer y
programar los módulos y casos de prueba. Se debe declarar este uso según las
reglas académicas, comprender las decisiones e inspeccionar los resultados.
