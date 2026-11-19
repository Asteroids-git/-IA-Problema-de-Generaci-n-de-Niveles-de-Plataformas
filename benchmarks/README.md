# Rendimiento del verificador de completabilidad

**Recomendación.** Empezar Simulated Annealing con el verificador exacto. En la carga oficial medida, 10.000 llamadas secuenciales representan unos 12 s a nivel de verificación. Un candidato con un tablero denso puede costar un orden de magnitud más. Conviene volver a medir sobre la distribución de vecinos que produzcan los operadores cuando existan.

## Reproducir

Desde la raíz del proyecto, en Linux con C++17 y Python 3:

```sh
make benchmark
```

La opción de comparación vuelve a comprobar completabilidad, número mínimo de movimientos, apoyos distintos alcanzados y aristas potenciales de cada caso de la versión base que finalizó:

```sh
python3 benchmarks/ejecutar.py --comparar benchmarks/firmas_base.csv
```

El comando genera `build/benchmark/{tiempos,memoria,resumen}.csv`. La medición archivada se hizo con `--cpu 6 --timeout 8` en un AMD Ryzen 5 5600H, x86-64, GCC 11.4/libstdc++, `-O2`. Se usaron las semillas 0 a 63, cinco repeticiones por semilla e instancia y siete por caso sintético. La versión base es el verificador previo con los mismos contadores añadidos sin cambiar su búsqueda. En la [comparación intercalada](mediciones_intercaladas.csv) se alternó qué versión iba primero durante seis rondas, tres repeticiones por semilla e instancia en cada ronda.

El tiempo de pared incluye construir y liberar el grafo, buscar y reconstruir el camino. Se excluyen la generación, la lectura y la escritura. El p95 es el rango más cercano superior. Los estados expandidos cuentan aquellos cuyas opciones se recorren en la búsqueda con máscaras. Cero indica que las cotas bastaron en todos los casos con ese valor. La memoria se mide en una pasada aparte con una repetición por nivel. Se registra el pico de bytes simultáneamente vivos solicitados al heap por la llamada, incluidos los búferes de los vectores en las reasignaciones. La cifra excluye pila, granularidad del asignador, instrumentación y memoria base del proceso. Los casos sin muestra completa se marcan como censurados por tiempo.

## Complejidad

Sea `C = 150` el ancho, `P` el número de apoyos, `R = 6` el alcance horizontal, `A ≤ 2RP` el número de aristas potenciales y `Q` la suma de sus opciones. Sea `W` el número de palabras de una máscara, tres en el binario medido.

- **Grafo.** Se comprueban hasta `2RP` pares. Cada par enumera como máximo unas 300 trayectorias candidatas, con salida anticipada en algunos casos. Un barrido visita `O(R)` columnas. Incorporar una opción puede comparar hasta `K` opciones ya retenidas en ese par en `O(KW)`. Con `T` candidatos por par, una cota de construcción es `O(C + PRT(R + KW))`. El grafo persistente ocupa `O(C + A + QW)` y cada par puede usar espacio temporal para sus opciones.
- **Cotas de la versión base.** Las dos búsquedas por columna cuestan `O(C + A + QW)` y usan `O(C)` memoria adicional. La inferior usa movimientos garantizados desde cada apoyo. La superior admite cualquier requisito. Si alcanzan los mismos apoyos y obtienen la misma distancia a la meta, certifican ambos resultados. Si hay que buscar con máscaras, la propagación de relevancia cuesta `O(W(Q + C(C + A)))` en el peor caso. Su número de rondas depende de la longitud de los caminos por los que se propagan los requisitos.
- **Búsqueda con máscaras de la versión base.** Si `rᵥ` es la cantidad de bits relevantes en la columna `v`, podían insertarse hasta `Σᵥ 2^rᵥ` estados distintos. Se comprueban las opciones salientes de cada estado expandido y, por opción habilitada, se recorre hasta dos veces la frontera del destino para probar subconjuntos y superconjuntos. Con `S` estados insertados, `X` expandidos, `Qmax` opciones salientes máximas y `Fmax` tamaño máximo de frontera, el peor tiempo de la búsqueda es `O(S + X Qmax W (1 + Fmax))`. En la versión base la frontera de una columna guarda máscaras incomparables por inclusión, aunque un subconjunto anterior puede seguir pendiente de expansión para preservar una ruta más corta. El espacio total es `O(C + A + WQ + (W + 1)S)`. Los estados inactivos conservan su registro para reconstruir padres. En esta compilación, una máscara ocupa 24 bytes y un estado 48 bytes, antes de contar los índices, las reservas de capacidad y el grafo.
- **Cambios medidos.** Cuando las primeras cotas discrepan, la nueva cota superior repite la búsqueda por columna reuniendo los pisotones de todas las rutas posibles en una máscara común. Añade hasta `E + 1` búsquedas para `E` enemigos. La unión puede conceder eliminaciones de rutas incompatibles, por lo que solo acota lo alcanzable. La fase exacta usa A* con distancias del grafo optimista. Una máscara puede dominar a otra mientras se busca la ruta mínima solo si llega en igual o menor cantidad de pasos. La cola añade `O(S log S)` tiempo y `O(S)` espacio para `S` inserciones. Las fronteras, las expansiones y la memoria siguen pudiendo crecer exponencialmente.

## Resultados

Los tiempos están en ms y corresponden a `mediana / p95 / máximo`. Los estados son expansiones máximas de máscaras y el heap es el pico máximo en KiB. Los CSV incluyen los tres estadísticos de estados y memoria, los estados insertados y el porcentaje de tiempo dedicado al grafo. [Base](mediciones_base.csv) · [Actual](mediciones_actuales.csv)

| Instancia | Tiempo base | Tiempo actual | Estados base → actual | Heap base → actual |
|---:|---:|---:|---:|---:|
| 1 | 1,325 / 1,790 / 2,325 | 1,403 / 1,990 / 7,195 | 38 → 0 | 20,320 → 18,578 |
| 2 | 0,558 / 0,749 / 1,069 | 0,573 / 0,836 / 1,171 | 22 → 0 | 7,523 → 7,484 |
| 3 | 1,326 / 1,769 / 2,436 | 1,380 / 2,003 / 4,201 | 38 → 0 | 20,320 → 18,578 |
| 4 | 2,578 / 3,227 / 4,466 | 2,632 / 3,578 / 5,130 | 310 → 0 | 69,227 → 34,969 |
| 5 | 0,544 / 0,776 / 1,004 | 0,566 / 0,784 / 1,142 | 22 → 0 | 7,523 → 7,484 |
| 6 | 0,584 / 0,827 / 1,086 | 0,601 / 0,861 / 1,214 | 5 → 0 | 8,438 → 8,438 |

En los lotes oficiales, construir el grafo consumió más del 99 % del tiempo. Las mediciones separadas y las intercaladas dieron diferencias de tiempo total en sentidos distintos. La comparación intercalada dio una reducción mediana de 2,13 µs por llamada en el tiempo fuera de la construcción del grafo. Los 384 niveles iniciales resultaron incompletables en ambas versiones. Las instancias 1 y 3 comparten tableros entre sí, igual que 2 y 5, porque el generador ignora el objetivo de linealidad.

| Estrés | Tiempo base | Tiempo actual | Estados base → actual | Heap base → actual |
|---|---:|---:|---:|---:|
| Denso, 74 enemigos | 32,874 / 42,250 / 42,250 | 33,709 / 37,513 / 37,513 | 0 → 0 | 217,125 → 217,125 |
| Bloqueo, 12 opcionales | 21,647 / 24,981 / 24,981 | 8,130 / 8,369 / 8,369 | 15.371 → 0 | 2.509,641 → 156,719 |
| Bloqueo, 16 opcionales | 349,993 / 377,911 / 377,911 | 11,200 / 11,881 / 11,881 | 87.023 → 0 | 9.540,141 → 158,594 |
| Bloqueo, 20 opcionales | >8.000 | 13,995 / 15,142 / 15,142 | — → 0 | — → 160,469 |
| Memoria, 12 opcionales | 24,377 / 25,600 / 25,600 | 8,997 / 9,818 / 9,818 | 18.559 → 40 | 2.474,492 → 135,992 |
| Memoria, 16 opcionales | 456,673 / 467,758 / 467,758 | 12,829 / 13,634 / 13,634 | 101.252 → 40 | 9.507,672 → 137,852 |
| Memoria, 20 opcionales | >8.000 | 15,468 / 16,924 / 16,924 | — → 40 | — → 139,711 |

El tablero denso tiene 150 apoyos a la misma altura. El de bloqueo combina un corredor con pisotones opcionales y un enemigo bajo el techo que solo se podría cruzar si hubiera desaparecido. El de memoria requiere pisar a un enemigo y volver a un apoyo anterior antes de alcanzar la meta. Hay datos adicionales para 4 y 8 enemigos opcionales en los CSV. Aislada, la nueva cota resolvió el bloqueo de 16 en unos 10–11 ms. El caso de memoria aún expandió 101.252 estados y tardó más de 430 ms hasta incorporar A*.

Se repitieron las 63 verificaciones de movimiento y alcanzabilidad, incluidos los 4.350 pares contra la referencia geométrica y los 150 niveles contra BFS exhaustiva. Una ejecución adicional amplió ese contraste a 1.000 niveles. Las 32 verificaciones del generador y las pruebas de las seis instancias también pasaron. Los 393 niveles de benchmark que finalizaron en ambos ejecutables coincidieron en completabilidad, pasos mínimos, apoyos alcanzados y aristas potenciales. Los dos casos censurados de la base completaron las mediciones tras los cambios.

## Aplicación a Simulated Annealing

La media de los seis lotes actuales fue 1,23 ms por llamada. Con esa mezcla, 1.000 evaluaciones son unos 1,2 s, 10.000 unas 12,3 s y 100.000 unos 123 s en un solo hilo. Diez mil evaluaciones de niveles como el tablero denso medido serían unos 5,7 min. Son extrapolaciones del costo del verificador. El costo completo y el p95 de SA dependerán de la distribución y correlación de los candidatos y del trabajo de los operadores.

Para comenzar, usaría el verificador exacto en cada evaluación y conservaría la ruta del mejor nivel factible. Una evaluación gradual puede aprovechar las cotas si más adelante solo se necesita el booleano. Una ruta garantizada prueba factibilidad, la ausencia de ruta bajo la cota superior prueba inviabilidad y el intervalo entre ambas exige búsqueda exacta. Para orientar SA se puede medir aparte una señal como el avance de la cota inferior o la distancia bajo la cota superior. Esa señal describe progreso y puede ordenar candidatos. La aceptación de un nivel como factible requiere una ruta certificada.

Si el perfil de SA muestra que la construcción del grafo domina, el siguiente experimento sería reutilizar aristas al cambiar pocas columnas o evaluar solo las que se visiten desde el inicio. El barrido de una arista depende de las celdas entre sus extremos y de sus apoyos. Cualquier reutilización debe invalidar los pares cuyo barrido incluya una columna modificada. El peor caso exponencial permanece, en particular si la cota superior combina eliminaciones que ninguna ruta real puede acumular.
