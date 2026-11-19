#!/usr/bin/env python3
"""Ejecuta el benchmark y guarda muestras crudas y un resumen por escenario."""

import argparse
import csv
import io
import math
from pathlib import Path
import statistics
import subprocess


def ejecutar(binario, escenario, repeticiones, cantidad, segundos, cpu):
    comando = [binario, escenario, str(repeticiones), str(cantidad)]
    if cpu is not None:
        comando = ["taskset", "-c", str(cpu), *comando]
    try:
        proceso = subprocess.run(comando, capture_output=True, text=True, timeout=segundos, check=True)
        salida, estado = proceso.stdout, "ok"
    except subprocess.TimeoutExpired as exp:
        salida = (exp.stdout or b"").decode()
        estado = f"timeout_{segundos}s"
    return list(csv.DictReader(io.StringIO(salida))), estado


def guardar(ruta, filas):
    if filas:
        with ruta.open("w", newline="") as archivo:
            escritor = csv.DictWriter(archivo, filas[0].keys())
            escritor.writeheader()
            escritor.writerows(filas)


def firmas(filas):
    resultados = {}
    for fila in filas:
        clave = tuple(fila[k] for k in ("escenario", "id", "semilla"))
        valor = tuple(fila[k] for k in ("completable", "pasos", "apoyos_alcanzados", "aristas_potenciales"))
        if resultados.setdefault(clave, valor) != valor:
            raise SystemExit(f"Resultado distinto al repetir {clave}")
    return resultados


def resumen(escenario, grupo, tiempos, memoria, estado_tiempo, estado_memoria):
    ms = sorted(int(fila["total_ns"]) / 1_000_000 for fila in tiempos)
    grafo_ns = sorted(int(fila["grafo_ns"]) for fila in tiempos)
    expandidos = sorted(int(fila["expandidos"]) for fila in tiempos)
    heap_bytes = sorted(int(fila["heap_bytes"]) for fila in memoria)

    def p95(ordenados):
        return ordenados[math.ceil(.95 * len(ordenados)) - 1]

    def tres(ordenados, factor=1):
        if not ordenados:
            return [""] * 3
        return [round(valor / factor, 3) for valor in (statistics.median(ordenados), p95(ordenados), ordenados[-1])]

    fila = {"escenario": escenario, "grupo": grupo, "muestras": len(ms),
            "tiempo": estado_tiempo, "memoria": estado_memoria,
            "certificados": sum(int(r["certificado"]) for r in tiempos),
            "media_ms": round(statistics.mean(ms), 3) if ms else "",
            "grafo_pct": round(100 * sum(grafo_ns) / sum(int(r["total_ns"]) for r in tiempos), 2) if ms else "",
            "insertados_max": max((int(r["insertados"]) for r in tiempos), default="")}
    for nombre, valores in (("ms", tres(ms)), ("expandidos", tres(expandidos)),
                            ("heap_kib", tres(heap_bytes, 1024))):
        for percentil, valor in zip(("p50", "p95", "max"), valores):
            fila[f"{nombre}_{percentil}"] = valor
    return fila


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tiempo", default="build/medir_alcanzabilidad")
    parser.add_argument("--heap", default="build/medir_alcanzabilidad_heap")
    parser.add_argument("--salida", type=Path, default=Path("build/benchmark"))
    parser.add_argument("--cpu", type=int, help="CPU lógica de Linux en la que se fija la afinidad")
    parser.add_argument("--timeout", type=int, default=30, help="segundos por proceso de medición")
    parser.add_argument("--semillas", type=int, default=64)
    parser.add_argument("--comparar", type=Path, help="tiempos.csv de una medición anterior")
    args = parser.parse_args()
    args.salida.mkdir(parents=True, exist_ok=True)
    casos = [("oficiales", args.semillas, 5), ("denso", 0, 7)]
    casos.extend((escenario, enemigos, 7) for escenario in ("bloqueo", "memoria")
                 for enemigos in (4, 8, 12, 16, 20))
    resumenes, tiempos, memoria = [], [], []
    for escenario, cantidad, repeticiones in casos:
        medidas, estado_t = ejecutar(args.tiempo, escenario, repeticiones, cantidad, args.timeout, args.cpu)
        heaps, estado_m = ejecutar(args.heap, escenario, 1, cantidad, args.timeout, args.cpu)
        tiempos.extend(medidas)
        memoria.extend(heaps)
        grupos = range(1, 7) if escenario == "oficiales" else [74 if escenario == "denso" else cantidad]
        for grupo in grupos:
            grupo_t = [fila for fila in medidas if int(fila["id"]) == grupo]
            grupo_m = [fila for fila in heaps if int(fila["id"]) == grupo]
            resumenes.append(resumen(escenario, grupo, grupo_t, grupo_m, estado_t, estado_m))
            ultimo = resumenes[-1]
            print(escenario, grupo, estado_t, "p50 ms", ultimo["ms_p50"], "estados máx.", ultimo["expandidos_max"], flush=True)
    guardar(args.salida / "tiempos.csv", tiempos)
    guardar(args.salida / "memoria.csv", memoria)
    guardar(args.salida / "resumen.csv", resumenes)
    actual = firmas(tiempos)
    for clave, valor in firmas(memoria).items():
        if clave in actual and actual[clave] != valor:
            raise SystemExit(f"Resultado distinto en la medición de memoria para {clave}")
    if args.comparar:
        with args.comparar.open(newline="") as archivo:
            previo = firmas(csv.DictReader(archivo))
        diferencias = [clave for clave, valor in previo.items() if actual.get(clave) != valor]
        if diferencias:
            raise SystemExit(f"Casos previos ausentes o distintos ({len(diferencias)}). Primero {diferencias[0]}")
        print(f"Coinciden {len(previo)} casos previos. Casos adicionales completos {len(actual.keys() - previo.keys())}")


if __name__ == "__main__":
    main()
