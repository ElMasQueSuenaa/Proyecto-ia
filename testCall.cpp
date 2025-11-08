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

template <class T>
T clamp(T v, T lo, T hi) { return std::max(lo, std::min(v, hi)); }

float frand(float a, float b) {
    std::uniform_real_distribution<float> D(a, b);
    return D(rng);
}
int irand(int a, int b) {
    std::uniform_int_distribution<int> D(a, b);
    return D(rng);
}
int wrapRotation(int deg) {
    int v = deg % 360;
    return (v < 0) ? (v + 360) : v;
}

// -------------------- Métrica --------------------
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
void sampleTargetRGB(const Canvas& target, float x_rel, float y_rel,
                     int& r, int& g, int& b) {
    int x = clamp(int(std::round(x_rel * (target.width  - 1))),  0, target.width  - 1);
    int y = clamp(int(std::round(y_rel * (target.height - 1))), 0, target.height - 1);
    int idx = (y * target.width + x) * 3;
    r = target.rgb[idx + 0];
    g = target.rgb[idx + 1];
    b = target.rgb[idx + 2];
}

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

Stroke randomStroke(const Canvas& target) {
    float x = frand(0.02f, 0.98f);
    float y = frand(0.02f, 0.98f);
    float size = frand(0.02f, 0.3f); // tamaño pincelada
    float rot  = frand(0.0f, 360.0f);
    int type   = irand(0, BRUSH_MAX_TYPE());
    int r, g, b; sampleTargetRGB(target, x, y, r, g, b);
    return Stroke(x, y, size, rot, type, r, g, b);
}

Stroke perturb(const Stroke& s, const Steps& st) {
    Stroke t = s;
    switch (irand(0, 6)) { // 0:x 1:y 2:size 3:rot 4:r 5:g 6:b
        case 0: t.x_rel = clamp(t.x_rel + frand(-st.posStep,  st.posStep),  0.0f, 1.0f); break;
        case 1: t.y_rel = clamp(t.y_rel + frand(-st.posStep,  st.posStep),  0.0f, 1.0f); break;
        case 2: t.size_rel = clamp(t.size_rel + frand(-st.sizeStep, st.sizeStep), 0.02f, 1.50f); break;
        case 3: t.rotation_deg = float(wrapRotation(int(std::round(t.rotation_deg)) + (irand(0,1)? st.rotStep : -st.rotStep))); break;
        case 4: t.r = clamp(t.r + (irand(0,1)? st.colorStep : -st.colorStep), 0, 255); break;
        case 5: t.g = clamp(t.g + (irand(0,1)? st.colorStep : -st.colorStep), 0, 255); break;
        case 6: t.b = clamp(t.b + (irand(0,1)? st.colorStep : -st.colorStep), 0, 255); break;
    }
    if (frand(0.f, 1.f) < st.typeProb && BRUSH_MAX_TYPE() > 0) {
        t.type = irand(0, BRUSH_MAX_TYPE());
    }
    return t;
}

// Evalúa un conjunto completo (seguro y simple)
double evalStrokes(const std::vector<Stroke>& S, const Canvas& target) {
    Canvas tmp(target.width, target.height);
    render(S, tmp);
    return mse(tmp, target);
}

// -------------------- Hill Climbing (Best-Improvement) --------------------
struct HCParams {
    int T = 340;          // trazos
    int iters = 5000;    // iteraciones
    int stall_limit = 1500;// corte por estancamiento
    int K = 32;           // vecinos por iteración (best-of-K)
    Steps steps;          // magnitudes del vecindario
    bool cool_steps = true;// enfriamiento de pasos
};

static Steps baseSteps; // para cooling

void applyCooling(Steps& st, const HCParams& P, int it) {
    if (!P.cool_steps) return;
    float progress = float(it) / std::max(1, P.iters); // 0..1
    float cool = 1.0f - 0.7f * progress;              // hasta 30% del valor inicial
    st.posStep   = baseSteps.posStep   * cool;
    st.sizeStep  = baseSteps.sizeStep  * cool;
    st.rotStep   = std::max(4, int(std::round(baseSteps.rotStep * cool)));
    st.colorStep = std::max(6, int(std::round(baseSteps.colorStep * cool)));
    st.typeProb  = std::max(0.05f, baseSteps.typeProb * (0.8f + 0.2f * cool));
}

// Añadimos logging: pasamos startTime y un ostream opcional
double hillClimbBest(std::vector<Stroke>& Sbest,
                     const Canvas& target,
                     HCParams& P,
                     const std::chrono::high_resolution_clock::time_point& t0,
                     std::ostream* log = nullptr,
                     int log_every = 500) {
    // init steps baseline
    baseSteps = P.steps;

    // 1) solución inicial
    Sbest.clear(); Sbest.reserve(P.T);
    for (int i = 0; i < P.T; ++i) Sbest.push_back(randomStroke(target));
    double best = evalStrokes(Sbest, target);

    // fila 0 (estado inicial)
    if (log) {
        double secs0 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
        (*log) << 0 << '\t' << secs0 << '\t' << best << '\n';
    }

    int stall = 0;
    for (int it = 1; it <= P.iters; ++it) {
        // cooling de pasos
        applyCooling(P.steps, P, it);

        // Round-robin del índice de stroke a mejorar
        int i = (it - 1) % P.T;

        // Generar K vecinos y quedarnos con la mejor mejora
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

        // LOG: cada mejora y también cada log_every iteraciones
        if (log && (improved || (it % log_every == 0))) {
            double secs = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
            (*log) << it << '\t' << secs << '\t' << best << '\n';
        }

        if (it % 500 == 0 || improved) {
            std::cout << "[it " << it << "] MSE=" << best
                      << (improved ? "  (mejora)\n" : "\n");
        }
        if (stall >= P.stall_limit) {
            std::cout << "⛳  Corte por estancamiento (" << stall << ")\n";
            break;
        }
    }
    return best;
}

// -------------------- main --------------------
int main(int argc, char** argv) {
    // Args: target [T] [iters] [seed] [K] [logfile]
    std::string targetPath = (argc >= 2) ? argv[1] : "instancias/bach.png";

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

    std::cout << "✅ MSE final: " << best << "\n";
    std::cout << "🖼️ Guardado: output.png\n";
    std::cout << "📝 Log guardado en: " << logFile << "\n";
    std::cout << "⏱️ Tiempo total: " << secs << " s\n";
    return 0;
}
