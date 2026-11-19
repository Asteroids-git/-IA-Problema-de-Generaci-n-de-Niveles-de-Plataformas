# Simulated Annealing y resultados preliminares

## Evaluación y vecindario

La búsqueda minimiza la suma ponderada de cuatro términos:

| Término | Definición | Peso inicial |
|---|---|---:|
| Error de cantidades | Suma de las diferencias absolutas de plataformas, enemigos y obstáculos | 0,07 por elemento |
| Penalización de linealidad | `1 − L` para `max` y `L` para `min` | 0,5 |
| Incompletabilidad | `1` si el verificador exacto determina que la meta es inalcanzable | 1,5 |
| Progreso pendiente | `1 − progreso` | 3,0 |

Para un nivel incompleto, `progreso = 0,9 × avance + 0,1 × cobertura`. El avance es `1 − distancia_restante / distancia_inicial`. La distancia restante se mide desde la columna alcanzable más cercana a la meta y la inicial entre los extremos. La cobertura es el número de columnas alcanzadas dividido por 150. Un certificado de completabilidad fija el progreso en 1. Avanzar cuatro columnas aporta alrededor de 0,07 al costo con un tablero de este ancho, por lo que puede compensar un elemento de diferencia temporal. La cobertura distingue niveles que llegan igual de cerca. La penalización binaria favorece completar el último tramo. Entre soluciones factibles se comparan cantidades y linealidad según sus pesos. Son escalas iniciales para experimentar.

El generador existente construye el candidato inicial con los conteos solicitados. Las propuestas cambian alturas, trasladan plataformas con su peligro, mueven peligros o agregan y quitan elementos. Quitar una plataforma también quita su peligro. Los extremos conservan sus columnas dentro de las bandas y su altura puede cambiar. Las propuestas dirigidas preparan un apoyo hacia la meta desde una columna alcanzable o acercan un conteo a su valor solicitado. Después de certificar una ruta, la probabilidad reservada a extenderla puede pasar a corregir conteos. Si una propuesta dirigida falla o resulta inaplicable, se prueba el vecindario general.

Se verifica la estructura y se ejecuta el verificador exacto una vez por candidato inicial y vecino evaluado. Se aceptan mejoras y empates, y se aceptan empeoramientos con probabilidad `exp(−Δ/T)`. La temperatura cae geométricamente de 0,08 a 0,002 en 3.000 iteraciones. Se archiva el mejor factible entre todos los evaluados, incluidos los rechazados. Cuando la corrida agota el presupuesto sin encontrar un factible, devuelve el mejor incompleto identificado como tal. En ese caso el reporte registra `pasos=-1`.

## Reproducción

Desde la raíz del proyecto:

```sh
make
./plataformas instancias.txt --sa --id 4 --semilla 42 --iteraciones 3000
python3 experimentos/correr_sa.py --semillas 0 42 12345
```

`./plataformas --help` enumera los parámetros. El script registra los pesos, las temperaturas, las probabilidades y los resultados por corrida en `build/experimentos-sa/resultados.csv`. Los reportes individuales con el tablero y la ruta certificada quedan en `build/experimentos-sa/niveles`. `--salida` y `--reportes` permiten distinguir experimentos. Con la misma semilla, instancia, configuración y biblioteca estándar se reproducen el nivel y las decisiones de búsqueda. Las mediciones de tiempo varían entre ejecuciones.

## Resultados y límites

El [CSV de las corridas](sa_preliminar.csv) registra cantidades, error, linealidad inicial y final, completabilidad, longitud del camino en movimientos, tiempo, evaluaciones y primera iteración factible. Se usaron las semillas 0, 42 y 12345, 3.000 iteraciones por corrida y una CPU lógica en un AMD Ryzen 5 5600H con GCC 11.4 y `-O2`.

Las 18 corridas devolvieron un nivel completable con las cantidades exactas. Cada corrida evaluó el candidato inicial y los 3.000 vecinos. La primera solución factible apareció entre las iteraciones 22 y 86. Una comprobación adicional leyó los tableros de los 18 reportes, repitió el verificador y reprodujo los 18 caminos guardados.

| Instancia | Objetivo | Linealidad final | Movimientos del camino | Tiempo (s) |
|---:|:---:|---:|---:|---:|
| 1 | max | 0,783–0,808 | 30–32 | 4,856–5,201 |
| 2 | max | 0,865–0,884 | 29–31 | 2,113–2,248 |
| 3 | min | 0,549–0,586 | 29–33 | 4,735–4,824 |
| 4 | min | 0,538–0,616 | 31–32 | 7,029–8,586 |
| 5 | min | 0,495–0,701 | 29–30 | 1,988–2,213 |
| 6 | min | 0,565–0,595 | 29–30 | 2,187–2,444 |

El verificador ocupó el 99,8 % del tiempo de búsqueda. En las 54.018 evaluaciones midió una media de 1,36 ms por llamada y un máximo de 7,46 ms. Estos tiempos permiten experimentar con el presupuesto ensayado. La ejecución usa el verificador exacto en cada evaluación.

Todos los niveles iniciales de la muestra eran incompletables. En 17 corridas la linealidad final también mejoró en la dirección de la instancia respecto al inicio. La instancia 5 con semilla 42 pasó de 0,638 a 0,701 al completar la ruta con los conteos solicitados. Esa comparación incluye el cambio de factibilidad. El presupuesto de iteraciones puede agotarse antes de encontrar una ruta en otras instancias o semillas. Los pesos, la frecuencia de propuestas y las temperaturas quedan pendientes de calibración.
