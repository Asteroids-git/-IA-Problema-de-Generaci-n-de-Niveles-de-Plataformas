#!/usr/bin/env bash
set -euo pipefail

# Prueba las 6 instancias: lectura, generación, estructura y conteos.
for id in 1 2 3 4 5 6; do
  salida=$(./plataformas instancias.txt --id "$id" --semilla 42)
  echo "$salida" | grep -q 'Restricciones estructurales: VALIDAS'
  echo "$salida" | grep -q 'Error cantidades=0'
  echo "$salida" | grep -Eq 'Completabilidad: (COMPLETABLE|NO COMPLETABLE)'
  test -s "salidas/instancia_${id}.txt"
  echo "OK: instancia $id"
done

# La selección interactiva acepta ID y termina al recibir 0.
printf '2\n0\n' | ./plataformas instancias.txt > /tmp/plataformas_menu_$$.log
grep -q 'INSTANCIA 2' /tmp/plataformas_menu_$$.log
rm -f /tmp/plataformas_menu_$$.log

# El modo SA conserva la entrada por ID y registra una evaluacion por vecino.
salida=$(./plataformas instancias.txt --id 1 --sa --semilla 4294967295 --iteraciones 4 \
  --temperatura-inicial 0.1 --temperatura-final 0.01 --directorio-salidas build/salidas-prueba)
grep -q 'SA iteraciones=4 | evaluaciones=5' <<< "$salida"
test -s build/salidas-prueba/instancia_1_sa_4294967295.txt
if ./plataformas instancias.txt --id 1 --sa --temperatura-final 1 >/dev/null 2>&1; then
  echo 'ERROR: SA no detecto temperaturas incompatibles'
  exit 1
fi

# Las entradas inválidas deben fallar con un error (sin segfault).
archivo_invalido="/tmp/plataformas_instancias_invalidas_$$.txt"
printf '1 75 20 15 max\n1 50 20 15 min\n' > "$archivo_invalido"
if ./plataformas "$archivo_invalido" --id 1 > /dev/null 2>&1; then
  echo 'ERROR: no detecto ID duplicado'
  rm -f "$archivo_invalido"
  exit 1
fi
rm -f "$archivo_invalido"
echo 'Todas las pruebas automatizadas superadas.'
