#include "main.h"
#include "core/SortAlgo.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <utility>
#include <vector>


int main()
{
    using Words = std::vector<std::string>;
    using Clock = std::chrono::steady_clock;
 
    std::mt19937 rng(42);
    volatile size_t sink = 0;
 
    auto makeData = [&](size_t count, size_t minLen, size_t maxLen, int accentPct) {
        static const char* accents[] = { "é","è","ê","à","â","ç","î","ô","ù","û","É","È","À","Ç" };
        Words v;
        v.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            std::string w;
            size_t len = minLen + rng() % (maxLen - minLen + 1);
            for (size_t k = 0; k < len; ++k)
            {
                if (static_cast<int>(rng() % 100) < accentPct)
                    w += accents[rng() % 14];
                else
                {
                    char c = static_cast<char>('a' + rng() % 26);
                    if (rng() % 4 == 0) c = static_cast<char>(c - 32);
                    w += c;
                }
            }
            v.push_back(std::move(w));
        }
        return v;
    };
 
    // --- chronométrage -------------------------------------------------------
    auto ms = [](Clock::time_point a, Clock::time_point b) {
        return std::chrono::duration<double, std::milli>(b - a).count();
    };
 
    // Une seule exécution, sans échauffement (froid)
    auto cold = [&](auto fn) {
        auto t0 = Clock::now();
        fn();
        return ms(t0, Clock::now());
    };
 
    // Warm-up puis médiane (chaud)
    auto hot = [&](auto fn, int warmups, int reps) {
        for (int i = 0; i < warmups; ++i) fn();
        std::vector<double> t;
        for (int i = 0; i < reps; ++i)
        {
            auto t0 = Clock::now();
            fn();
            t.push_back(ms(t0, Clock::now()));
        }
        std::sort(t.begin(), t.end());
        return t[t.size() / 2];
    };
 
    // --- scénarios -------------------------------------------------------------
    struct Scenario { const char* name; size_t count, minLen, maxLen; int accent; };
    const Scenario scenarios[] = {
        { "Mots courts + accents  (SIMD inutile)", 5000,  3,  12, 10 },
        { "Textes longs + accents (SIMD partiel)", 5000, 40, 120, 10 },
        { "Textes longs sans accent (SIMD ideal)", 5000, 40, 120,  0 },
    };
 
    std::puts("Temps en ms (plus petit = mieux). Le tri SIMD utilise aussi les cles precalculees,");
    std::puts("donc 'Normalize seul' isole l'effet reel du SSE2.\n");
 
    for (const Scenario& sc : scenarios)
    {
        Words data = makeData(sc.count, sc.minLen, sc.maxLen, sc.accent);
 
        // On copie en dehors du chrono ; chaque lambda travaille sur sa propre copie
        auto sortNaive = [&] { Words w = data; SortAlgo::Sort(w);      sink = sink + w[0].size(); };
        auto sortSimd  = [&] { Words w = data; SIMDSortAlgo::Sort(w);  sink = sink + w[0].size(); };
 
        auto normNaive = [&] { size_t a = 0; for (auto& s : data) a += SortAlgo::Normalize(s).size();     sink = sink + a; };
        auto normSimd  = [&] { size_t a = 0; for (auto& s : data) a += SIMDSortAlgo::Normalize(s).size(); sink = sink + a; };
 
        std::printf("=== %s | N=%zu ===\n", sc.name, sc.count);
        std::printf("%-16s | %-10s | %-10s | %s\n", "", "sans SIMD", "avec SIMD", "gain");
        std::puts("-----------------+------------+------------+--------");
 
        // Froid : première exécution de chaque (note : ordre naïf -> SIMD, légèrement favorable au second)
        double cN = cold(sortNaive), cS = cold(sortSimd);
        std::printf("%-16s | %8.2f   | %8.2f   | x%.2f\n", "Tri   (froid)", cN, cS, cN / cS);
 
        double cNn = cold(normNaive), cNs = cold(normSimd);
        std::printf("%-16s | %8.2f   | %8.2f   | x%.2f\n", "Normalize (froid)", cNn, cNs, cNn / cNs);
 
        // Chaud : 3 warm-up + médiane de 11 (moins pour les gros tris)
        double hN = hot(sortNaive, 2, 5), hS = hot(sortSimd, 2, 5);
        std::printf("%-16s | %8.2f   | %8.2f   | x%.2f\n", "Tri   (chaud)", hN, hS, hN / hS);
 
        double hNn = hot(normNaive, 3, 11), hNs = hot(normSimd, 3, 11);
        std::printf("%-16s | %8.2f   | %8.2f   | x%.2f\n\n", "Normalize (chaud)", hNn, hNs, hNn / hNs);
    }
 
    return 0;
}
