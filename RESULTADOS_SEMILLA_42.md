# Resultados de v3 con semilla 42

La version 3 tiene generador inicial aleatorio independiente de `max/min`. Todos los resultados se calcularon ejecutando el programa, no son estimaciones.

| ID | P/E/O generados | Linealidad | Costo | Completabilidad |
|---:|---|---:|---:|---|
| 1 | 75/20/15 | 0.6649 | 0.1676 | NO COMPLETABLE |
| 2 | 50/20/15 | 0.6381 | 0.1810 | NO COMPLETABLE |
| 3 | 75/20/15 | 0.6649 | 0.3324 | NO COMPLETABLE |
| 4 | 100/20/15 | 0.6579 | 0.3290 | NO COMPLETABLE |
| 5 | 50/20/15 | 0.6381 | 0.3190 | NO COMPLETABLE |
| 6 | 75/15/35 | 0.6649 | 0.3324 | NO COMPLETABLE |

Las instancias con iguales cantidades y semilla (1 y 3; 2 y 5) producen el mismo tablero inicial, pero su **costo** cambia porque la funcion de evaluacion usa el objetivo de linealidad.

Los resultados de v2 mostrados previamente en diapositivas 6, 7 y 9 deben reemplazarse por estos valores si se presenta v3.

La finalizacion del nivel sigue sin estar garantizada: `verificarCompletabilidad` reporta el resultado del modelo simplificado.
