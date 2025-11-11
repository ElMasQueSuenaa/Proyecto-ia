import os
import re
from typing import List, Tuple
import matplotlib.pyplot as plt

def parse_log(filepath: str) -> Tuple[List[float], List[float]]:
    """
    Lee un archivo .txt con columnas iter, segundos, MSE y retorna dos listas:
    tiempos y MSEs.
    """
    times = []
    mses = []

    time_candidates = {"segundos", "tiempo", "time", "seconds", "sec"}
    mse_candidates = {"mse", "error", "mse_best", "msefinal", "mse_final"}

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        header_idx_map = None
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue

            parts = re.split(r"[, \t]+", line)

            # Detectar encabezado
            if header_idx_map is None:
                lower_parts = [p.lower() for p in parts]
                t_idx = None
                m_idx = None
                for i, p in enumerate(lower_parts):
                    if p in time_candidates and t_idx is None:
                        t_idx = i
                    if p in mse_candidates and m_idx is None:
                        m_idx = i
                if t_idx is not None and m_idx is not None:
                    header_idx_map = (t_idx, m_idx)
                    continue

            # Leer datos numéricos
            try:
                if header_idx_map is not None:
                    t_idx, m_idx = header_idx_map
                    t = float(parts[t_idx])
                    m = float(parts[m_idx])
                else:
                    if len(parts) < 3:
                        if len(parts) >= 2:
                            t = float(parts[0])
                            m = float(parts[1])
                        else:
                            continue
                    else:
                        t = float(parts[1])
                        m = float(parts[2])
                times.append(t)
                mses.append(m)
            except ValueError:
                continue

    if not times or not mses:
        raise ValueError("No se pudieron extraer columnas de tiempo y MSE. Revisa el formato del archivo.")

    return times, mses


def main():
    print("=== Gráfico MSE vs Tiempo ===")
    filepath = r"C:\Users\beiker\Documents\GitHub\Proyecto-ia\logs\starrynight_2025-11-07_19-02-34.txt"

    if not os.path.exists(filepath):
        print("El archivo no existe. Verifica la ruta e inténtalo de nuevo.")
        return

    times, mses = parse_log(filepath)

    plt.figure()
    plt.plot(times, mses, linewidth=2)
    plt.xlabel("Tiempo (s)")
    plt.ylabel("MSE")
    plt.title("MSE vs Tiempo")
    plt.grid(True)

    base, _ = os.path.splitext(filepath)
    out_path = base + "_mse_vs_tiempo.png"
    plt.savefig(out_path, dpi=160, bbox_inches="tight")

    print(f"Gráfico guardado en: {out_path}")

if __name__ == "__main__":
    main()
