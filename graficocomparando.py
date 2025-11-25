import os
import re
import pandas as pd
from io import StringIO
from pathlib import Path

import numpy as pd
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


# ------------ LECTURA DE LOGS ------------

def cargar_log(path):
    """
    Lee un log y extrae SOLO líneas con formato:
    <int> <float> <float>

    Ignora:
    - encabezados
    - líneas con texto
    - líneas con más columnas
    - # comentarios
    """
    path = Path(path)

    data = []

    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()

            # ignorar vacío y comentarios
            if not line or line.startswith("#"):
                continue

            # buscar: entero + float + float
            m = re.match(r"^(\d+)\s+([\d\.Ee+-]+)\s+([\d\.Ee+-]+)", line)
            if m:
                iter_val = int(m.group(1))
                tiempo = float(m.group(2))
                mse = float(m.group(3))
                data.append((iter_val, tiempo, mse))

    return pd.DataFrame(data, columns=["iter", "segundos", "MSE"])


def obtener_config(path):
    """
    Detecta la configuración a partir de la ruta.
    Ajusta aquí los patrones si cambias los nombres de carpeta.
    """
    path = str(path)
    if "240TR+5000IT" in path:
        return "A (240, 5000)"
    if "340TR+5000IT" in path:
        return "B (340, 5000)"
    if "340TR+10000IT" in path:
        return "C (340, 10000)"
    return "Desconocida"


# ------------ PLOT PROMEDIO MSE VS TIEMPO ------------

def plot_promedio_mse_vs_tiempo(log_files, titulo="MSE vs tiempo (promedio por configuración)"):
    """
    Recibe una lista de rutas a logs, agrupa por configuración (A/B/C),
    interpola para tener un eje de tiempo común y grafica el MSE promedio.
    """

    # Agrupar DataFrames por configuración
    grupos = {}

    for f in log_files:
        if not os.path.exists(f):
            print(f"[ADVERTENCIA] No se encontró el archivo: {f}")
            continue

        config = obtener_config(f)
        df = cargar_log(f)

        if config not in grupos:
            grupos[config] = []
        grupos[config].append(df)

    if not grupos:
        print("No se pudo cargar ningún log. Revisa las rutas.")
        return

    plt.figure(figsize=(8, 5))

    for config, dfs in grupos.items():
        if not dfs:
            continue

        # tiempo máximo común (para poder interpolar todas en el mismo eje)
        t_max = min(df["segundos"].max() for df in dfs)
        # eje de tiempo "suavizado"
        t = np.linspace(0, t_max, 300)

        mses_interp = []
        for df in dfs:
            # interpolar MSE en el eje de tiempo común
            mse_interp = np.interp(t, df["segundos"], df["MSE"])
            mses_interp.append(mse_interp)

        # promedio sobre las distintas ejecuciones de esa configuración
        mse_prom = np.mean(mses_interp, axis=0)

        plt.plot(t, mse_prom, label=config)

    plt.xlabel("Tiempo (s)")
    plt.ylabel("MSE promedio")
    plt.title(titulo)
    plt.grid(alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.show()


# ------------ MAIN ------------

if __name__ == "__main__":
    log_files = [
        r"C:\Users\beiker\Documents\GitHub\Proyecto-ia\logs\240TR+5000IT\starrynight_2025-11-25_02-28-15.txt",
        r"C:\Users\beiker\Documents\GitHub\Proyecto-ia\logs\240TR+5000IT\starrynight_2025-11-25_02-34-15.txt",
        r"C:\Users\beiker\Documents\GitHub\Proyecto-ia\logs\240TR+5000IT\starrynight_2025-11-25_02-39-20.txt",
        r"C:\Users\beiker\Documents\GitHub\Proyecto-ia\logs\340TR+5000IT\starrynight_2025-11-07_15-49-46.txt",
        r"C:\Users\beiker\Documents\GitHub\Proyecto-ia\logs\340TR+5000IT\starrynight_2025-11-07_15-56-10.txt",
        r"C:\Users\beiker\Documents\GitHub\Proyecto-ia\logs\340TR+5000IT\starrynight_2025-11-23_00-13-50.txt",
    ]

    plot_promedio_mse_vs_tiempo(
        log_files,
        titulo="MSE vs tiempo para starrynight.png (promedio por configuración)"
    )
