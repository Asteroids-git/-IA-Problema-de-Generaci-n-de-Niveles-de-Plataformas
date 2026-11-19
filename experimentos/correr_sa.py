#!/usr/bin/env python3
"""Ejecuta SA secuencialmente para las seis instancias oficiales y registra un CSV."""
import argparse
import csv
from pathlib import Path
import re
import subprocess

PARAMETROS = {
    "temperatura-inicial": 0.08,
    "temperatura-final": 0.002,
    "prob-avance": 0.45,
    "prob-ajuste": 0.20,
    "peso-cantidades": 0.07,
    "peso-linealidad": 0.5,
    "peso-incompletabilidad": 1.5,
    "peso-progreso": 3.0,
}


def extraer(reporte, instancia, semilla):
    conteos = re.search(r"^Generado:\s+plataformas=(\d+) enemigos=(\d+) obstaculos=(\d+)$", reporte, re.M)
    error = re.search(r"^Error cantidades=(\d+)", reporte, re.M)
    factible = re.search(r"^Completabilidad: (COMPLETABLE|NO COMPLETABLE) ", reporte, re.M)
    objetivo = re.search(r"\| linealidad (max|min) \|", reporte)
    datos = dict(re.findall(r"(\w+)=([^\s|]+)", "\n".join(
        linea for linea in reporte.splitlines() if linea.startswith("SA "))))
    if not all((conteos, error, factible, objetivo)) or "verificador_segundos" not in datos:
        raise ValueError(f"Reporte SA incompleto para instancia {instancia}, semilla {semilla}")
    return {"instancia": instancia, "objetivo": objetivo[1], "semilla": semilla,
            "plataformas": conteos[1], "enemigos": conteos[2], "obstaculos": conteos[3],
            "error_cantidades": error[1], "completable": int(factible[1] == "COMPLETABLE"), **datos}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--semillas", nargs="+", type=int, default=[0, 42, 12345])
    parser.add_argument("--iteraciones", type=int, default=3000)
    parser.add_argument("--salida", type=Path, default=Path("build/experimentos-sa/resultados.csv"))
    parser.add_argument("--reportes", type=Path, default=Path("build/experimentos-sa/niveles"))
    for nombre, valor in PARAMETROS.items():
        parser.add_argument("--" + nombre, type=float, default=valor)
    args = parser.parse_args()
    parametros = {nombre.replace("-", "_"): getattr(args, nombre.replace("-", "_")) for nombre in PARAMETROS}
    opciones = [argumento for nombre in PARAMETROS
                for argumento in ("--" + nombre, str(parametros[nombre.replace("-", "_")]))]
    args.salida.parent.mkdir(parents=True, exist_ok=True)
    with args.salida.open("w", newline="") as archivo:
        escritor = None
        for instancia in range(1, 7):
            for semilla in args.semillas:
                comando = ["./plataformas", "instancias.txt", "--sa", "--id", str(instancia),
                           "--semilla", str(semilla), "--iteraciones", str(args.iteraciones),
                           "--directorio-salidas", str(args.reportes), *opciones]
                texto = subprocess.run(comando, check=True, capture_output=True, text=True).stdout
                fila = {**extraer(texto, instancia, semilla), **parametros}
                if escritor is None:
                    escritor = csv.DictWriter(archivo, fieldnames=fila.keys(), lineterminator="\n")
                    escritor.writeheader()
                escritor.writerow(fila)
                archivo.flush()
                print(f"Instancia {instancia} semilla {semilla} factible={fila['completable']} "
                      f"error={fila['error_cantidades']} L={fila['linealidad']} tiempo={fila['segundos']} s", flush=True)


if __name__ == "__main__":
    main()
