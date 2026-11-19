# Versión experimental ajustada tras retroalimentación del avance

Este repositorio es una COPIA de trabajo del código V3 presentado en octubre de 2026.
Los cambios se basan en el relato de las seis indicaciones del ayudante, y deben validarse
con más escenarios y pruebas antes de considerarse la versión de entrega final.

## Cambios realmente implementados en esta copia
- Inicio en 0..4 y meta en 145..149; separación >=141. Decisión de diseño.
- Bloques permitidos también en fila 15; NO son apoyo caminable y NO admiten peligro encima.
- Las cantidades del generador inicial SIGUEN siendo exactas por diseño.

## Modelo de movimiento y alcanzabilidad

- El personaje de 1×1 parte y termina centrado en un apoyo. Los saltos prueban
  elevaciones de 1 a 4 celdas y avances de hasta 6 columnas. La elevación se mide
  desde los pies al despegar. La caída de borde se evalúa por separado. El personaje
  camina hasta que su caja deja de solaparse con el apoyo y cae desde esa altura
  con velocidad vertical inicial cero. El límite horizontal incluye ese primer paso.
- En el aire, la aceleración vertical es constante en tiempo normalizado. El avance
  horizontal usa un perfil cosenoidal de velocidad continua, con inicio y duración
  ajustables y rapidez máxima de 6 celdas por unidad de ese tiempo. Se ensayan
  intervalos en octavos del tiempo de vuelo y perfiles alineados con el borde.
  El barrido continuo de la caja comprueba plataformas, peligros y límites.
- Se permite aterrizar exactamente en el ápice de un salto de altura máxima sobre
  una plataforma sólida si el cuerpo alcanza su borde o superficie con trayectoria
  libre. El personaje puede terminar de centrarse ya apoyado. Tocar un borde sin
  penetración está permitido. Un pisotón requiere que los pies lleguen desde arriba
  con velocidad vertical descendente y solapamiento horizontal positivo. Puede
  ocurrir sobre un enemigo intermedio y el descenso continúa sin rebote.
- Cada enemigo aplastado desaparece durante esa ruta. El verificador precalcula
  los requisitos y efectos de cada movimiento. Las cotas por columna pueden
  certificar la alcanzabilidad y la longitud mínima. Cuando discrepan, A* distingue
  estados por columna y enemigos eliminados y descarta estados dominados. Las aristas
  potenciales incluyen movimientos que requieren eliminaciones previas.

## Simulated Annealing

```sh
./plataformas instancias.txt --sa --id 4 --semilla 42 --iteraciones 3000
./plataformas --help
```

El modo `--sa` valida cada vecino con el verificador exacto y devuelve el mejor
nivel factible evaluado, si lo encuentra. La ejecución sin esa opción muestra
el nivel del generador inicial. Los reportes de SA incluyen la semilla en el
nombre. La función objetivo, los parámetros y las primeras mediciones se
describen en [experimentos](experimentos/README.md).

## Limitaciones conocidas

- Los pesos y el enfriamiento son parámetros preliminares pendientes de calibración.
- La familia de trayectorias tiene alturas y ajustes de tiempo discretos, avance
  monótono dentro de cada movimiento y enemigos estáticos. La búsqueda es exacta
  para esa familia y puede crecer exponencialmente con enemigos relevantes cuyas
  eliminaciones no se puedan ordenar por inclusión.

## Compilar y probar
```
make clean && make && make test
./plataformas instancias.txt --id 1 --semilla 42
```

El benchmark reproducible y el análisis de rendimiento están en [benchmarks](benchmarks/README.md).

## Diferencia con la formulación del estado del arte
El informe previo proponía segmentos de longitud variable/personaje puntual;
este código mantiene 1 bloque por columna/personaje de 1x1, y evalúa además
linealidad. El informe final debe describir fielmente esta representación.
