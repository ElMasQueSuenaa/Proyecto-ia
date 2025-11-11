// testCall.cpp — Hill Climbing + Mejor Mejora (Best-Improvement) + LOG a TXT
#include "stroke.h"
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <fstream>   // <<<<<<<<<<<<<<<<<<<<< NEW

// -------------------- RNG & utils --------------------
static std::mt19937 rng(std::random_device{}());

/**
 * Devuelve el valor @p v si se encuentra dentro del intervalo [@p lo, @p hi].
 * Si @p v es menor que @p lo devuelve @p lo; si @p v es mayor que @p hi
 * devuelve @p hi.
 *
 * @tparam T Tipo del valor y de los límites. Debe soportar comparaciones
 *           (operadores <, >) y copia/retorno por valor.
 * @param v   Valor a restringir.
 * @param lo  Límite inferior del intervalo.
 * @param hi  Límite superior del intervalo.
 * @return El valor ajustado al intervalo [lo, hi].
 *
 */
template <class T>
T clamp(T v, T lo, T hi) { return std::max(lo, std::min(v, hi)); }


/**
 * Genera y devuelve un valor de tipo float distribuido uniformemente en el
 * intervalo [a, b) — incluye a y excluye b.
 *
 * @param a Límite inferior del intervalo (incluido).
 * @param b Límite superior del intervalo (excluido).
 * @return Un valor float aleatorio en el intervalo [a, b). Si a == b devuelve a.
 *
 * @note La función utiliza un generador de números aleatorios externo llamado
 * rng 
 */
float frand(float a, float b) {
    std::uniform_real_distribution<float> D(a, b);
    return D(rng);
}


/**
 * Genera un entero aleatorio uniformemente distribuido en el intervalo [a, b].
 *
 * @param a Límite inferior del intervalo (inclusivo).
 * @param b Límite superior del intervalo (inclusivo).
 * @return int Valor entero seleccionado de forma uniforme en [a, b].
 *
 */
int irand(int a, int b) {
    std::uniform_int_distribution<int> D(a, b);
    return D(rng);
}


/**
 * Normaliza un ángulo en grados al rango [0, 359].
 *
 * @param deg Ángulo en grados; puede ser negativo o mayor que 360.
 * @return Entero en el rango 0..359 que representa el ángulo normalizado.
 *
 */
int wrapRotation(int deg) {
    int v = deg % 360;
    return (v < 0) ? (v + 360) : v;
}

// -------------------- Métrica --------------------
/**
 * Calcula el error cuadrático medio (MSE) entre dos imágenes representadas por Canvas.
 *
 * Esta función compara los valores de los píxeles de dos objetos Canvas A y B interpretando
 * los datos de color como una secuencia plana de componentes RGB.
 *
 * @param A Primer Canvas a comparar. Debe exponer miembros width, height y una secuencia rgb
 *          con al menos width*height*3 elementos.
 * @param B Segundo Canvas a comparar. Debe tener las mismas dimensiones que A.
 *
 * @return El valor de MSE como double. Si las dimensiones de A y B difieren, se escribe
 *         un mensaje de error en std::cerr ("MSE: tamaños distintos") y la función devuelve
 *         un valor sentinel muy grande (1e300) para indicar el fallo.
 *
 */
double mse(const Canvas& A, const Canvas& B) {
    if (A.width != B.width || A.height != B.height) {
        std::cerr << "MSE: tamaños distintos\n";
        return 1e300;
    }
    const size_t N = static_cast<size_t>(A.width) * A.height * 3;
    double acc = 0.0;
    for (size_t i = 0; i < N; ++i) {
        int d = int(A.rgb[i]) - int(B.rgb[i]);
        acc += double(d * d);
    }
    return acc / double(N);
}

// -------------------- Helpers de color --------------------
/**
 * Obtiene el color RGB de un píxel del lienzo objetivo en coordenadas.
 *
 * @param target Referencia constante al Canvas que contiene los campos 'width', 'height' y 'rgb'.
 *               Se asume que target.rgb es un arreglo contiguo con al menos width * height * 3 elementos
 *               y que el formato es R, G, B por píxel.
 * @param x_rel  Coordenada horizontal relativa (0.0 -> borde izquierdo, 1.0 -> borde derecho). Valores
 *               fuera de [0,1] se escalan y luego se clamp-ean a los límites del lienzo.
 * @param y_rel  Coordenada vertical relativa (0.0 -> borde superior, 1.0 -> borde inferior). Igual
 *               comportamiento de escalado y clamp que x_rel.
 * @param r      Referencia de salida donde se almacenará el componente rojo del píxel seleccionado.
 * @param g      Referencia de salida donde se almacenará el componente verde del píxel seleccionado.
 * @param b      Referencia de salida donde se almacenará el componente azul del píxel seleccionado.
 *
 */
void sampleTargetRGB(const Canvas& target, float x_rel, float y_rel,
                     int& r, int& g, int& b) {
    int x = clamp(int(std::round(x_rel * (target.width  - 1))),  0, target.width  - 1);
    int y = clamp(int(std::round(y_rel * (target.height - 1))), 0, target.height - 1);
    int idx = (y * target.width + x) * 3;
    r = target.rgb[idx + 0];
    g = target.rgb[idx + 1];
    b = target.rgb[idx + 2];
}

/**
 * Obtiene el índice máximo válido para los tipos de pincel.
 *
 * @return Índice máximo válido (entero >= 0) para los tipos de pincel.
 *
 */
int BRUSH_MAX_TYPE() {
    return std::max(0, int(gBrushes.size()) - 1); // 0..3 por defecto
}

// -------------------- Generación & Vecindario --------------------
struct Steps {
    float posStep   = 0.03f;
    float sizeStep  = 0.08f;
    int   rotStep   = 12;
    int   colorStep = 18;
    float typeProb  = 0.10f;
};


/**
 * hillClimbBest
 *
 * Realiza una búsqueda local tipo hill-climbing para optimizar un conjunto de trazos 
 * que aproximen un lienzo objetivo. 
 * Se itera sobre los trazos, generando hasta P.K perturbaciones por trazo en cada iteración, 
 * evaluando la calidad mediante evalStrokes y aplicando una política de "cooling" sobre el tamaño de las perturbaciones.
 *
 * Parámetros:
 * @param Sbest     Referencia a un vector de Stroke que contendrá la mejor solución encontrada.
 *                  Debe ser válido y se limpia/rellena por la función.
 * @param target    Canvas objetivo contra el que se evalúan las soluciones.
 * @param P         Parámetros de control (HCParams), contiene al menos: T, K, iters, steps,
 *                  stall_limit y otros necesarios para las funciones auxiliares.
 * @param t0        Punto temporal de referencia para cómputo de tiempos transcurridos usado en logs.
 * @param log       Puntero opcional a std::ostream para registrar (iteración, segundos, error).
 *                  Si es nullptr, no se escriben entradas de log en ese flujo.
 * @param log_every Frecuencia (en iteraciones) mínima para escribir en log aun cuando no haya
 *                  mejoras (se combina con la escritura también cuando hay mejora).
 *
 * @return El valor de error (double, MSE) correspondiente a la mejor solución encontrada.
 *
 */
double hillClimbBest(std::vector<Stroke>& Sbest,
                     const Canvas& target,
                     HCParams& P,
                     const std::chrono::high_resolution_clock::time_point& t0,
                     std::ostream* log = nullptr,
                     int log_every = 500) {
    baseSteps = P.steps;

    // 1) solución inicial
    Sbest.clear(); Sbest.reserve(P.T);
    for (int i = 0; i < P.T; ++i) Sbest.push_back(randomStroke(target));
    double best = evalStrokes(Sbest, target);

    if (log) {
        double secs0 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
        (*log) << 0 << '\t' << secs0 << '\t' << best << '\n';
    }

    int stall = 0;
    for (int it = 1; it <= P.iters; ++it) {
        applyCooling(P.steps, P, it);
        int i = (it - 1) % P.T;
        double localBest = best;
        Stroke bestNeighbor = Sbest[i];
        bool improved = false;

        for (int k = 0; k < P.K; ++k) {
            Stroke cand = perturb(Sbest[i], P.steps);
            std::vector<Stroke> Stry = Sbest;
            Stry[i] = cand;
            double val = evalStrokes(Stry, target);
            if (val + 1e-12 < localBest) {
                localBest = val;
                bestNeighbor = cand;
                improved = true;
            }
        }

        if (improved) {
            Sbest[i] = bestNeighbor;
            best = localBest;
            stall = 0;
        } else {
            ++stall;
        }

        if (log && (improved || (it % log_every == 0))) {
            double secs = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
            (*log) << it << '\t' << secs << '\t' << best << '\n';
        }

        if (it % 500 == 0 || improved) {
            std::cout << "[it " << it << "] MSE=" << best
                      << (improved ? "  (mejora)\n" : "\n");
        }
        if (stall >= P.stall_limit) {
            std::cout << "Corte por estancamiento (" << stall << ")\n";
            break;
        }
    }
    return best;
}

// -------------------- main --------------------
int main(int argc, char** argv) {
    // Args: target [T] [iters] [seed] [K] [logfile]
    std::string targetPath = (argc >= 2) ? argv[1] : "instancias/starrynight.png";

    HCParams P;
    if (argc >= 3) P.T     = std::max(1, std::atoi(argv[2]));
    if (argc >= 4) P.iters = std::max(1, std::atoi(argv[3]));
    if (argc >= 5) {
        unsigned seed = (unsigned)std::stoul(argv[4]);
        rng.seed(seed);
        std::cout << "Semilla RNG: " << seed << "\n";
    }
    if (argc >= 6) P.K = std::max(1, std::atoi(argv[5]));

    // archivo de log (opcional)
    std::string logFile = (argc >= 7) ? argv[6] : "log.txt";

    // Ajustes de vecindario (puedes modificarlos aquí)
    P.steps.posStep   = 0.03f;
    P.steps.sizeStep  = 0.08f;
    P.steps.rotStep   = 12;
    P.steps.colorStep = 18;
    P.steps.typeProb  = 0.10f;
    P.stall_limit     = 1500;
    P.cool_steps      = true;

    // 1) Cargar brushes
    {
        ImageGray b0, b1, b2, b3;
        if (!loadImageGray("brushes/1.jpg", b0)) return 1;
        if (!loadImageGray("brushes/2.jpg", b1)) return 1;
        if (!loadImageGray("brushes/3.jpg", b2)) return 1;
        if (!loadImageGray("brushes/4.jpg", b3)) return 1;
        gBrushes.push_back(std::move(b0));
        gBrushes.push_back(std::move(b1));
        gBrushes.push_back(std::move(b2));
        gBrushes.push_back(std::move(b3));
    }

    // 2) Cargar objetivo
    Canvas target(1,1);
    if (!loadImageRGB_asCanvas(targetPath, target)) {
        std::cerr << "No se pudo cargar: " << targetPath << "\n";
        return 1;
    }
    std::cout << "Objetivo: " << targetPath << " (" << target.width << "x" << target.height << ")\n";
    std::cout << "T=" << P.T << "  iters=" << P.iters << "  K=" << P.K << "\n";

    // 3) Hill Climbing (Best-Improvement) con LOG
    auto t0 = std::chrono::high_resolution_clock::now();

    // abrir log en append y escribir cabecera
    std::ofstream log(logFile, std::ios::app);
    if (!log) {
        std::cerr << "No se pudo abrir " << logFile << " para escribir.\n";
        return 1;
    }
    log << "# target=" << targetPath
        << " T=" << P.T
        << " iters=" << P.iters
        << " K=" << P.K
        << " seed=" << (argc >= 5 ? argv[4] : "auto")
        << "\n";
    log << "iter\tsegundos\tMSE\n";

    std::vector<Stroke> bestS;
    double best = hillClimbBest(bestS, target, P, t0, &log, /*log_every=*/500);

    auto t1 = std::chrono::high_resolution_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();

    // 4) Render final y guardar
    Canvas C(target.width, target.height);
    render(bestS, C);
    if (!savePNG(C, "output.png")) {
        std::cerr << "Error guardando output.png\n";
        return 1;
    }

    // log final
    log << "# FIN\t" << secs << "\t" << best << "\n\n";
    log.close();

    std::cout << "MSE final: " << best << "\n";
    std::cout << "Guardado: output.png\n";
    std::cout << "Log guardado en: " << logFile << "\n";
    std::cout << "Tiempo total: " << secs << " s\n";
    return 0;
}
