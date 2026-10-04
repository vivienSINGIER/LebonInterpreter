#ifndef TEST_SORT_ALGO_HPP
#define TEST_SORT_ALGO_HPP

#include "core/SortAlgo.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/SearchAlgo.hpp"

namespace TestSortAlgo
{
    inline void RunBenchmark()
    {
        /*using Words = std::vector<std::string>;
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

        auto ms = [](Clock::time_point a, Clock::time_point b) {
            return std::chrono::duration<double, std::milli>(b - a).count();
        };

        auto cold = [&](auto fn) {
            auto t0 = Clock::now();
            fn();
            return ms(t0, Clock::now());
        };

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

        struct Scenario { const char* name; size_t count, minLen, maxLen; int accent; };
        const Scenario scenarios[] = {
            { "Mots courts + accents  (SIMD inutile)", 5000,  3,  12, 10 },
            { "Textes longs + accents (SIMD partiel)", 5000, 40, 120, 10 },
            { "Textes longs sans accent (SIMD ideal)", 5000, 40, 120,  0 },
        };

        std::puts("Temps en ms (plus petit = mieux). Le tri SIMD utilise aussi les cles precalculees,");
        std::puts("donc 'Normalize seul' isole l'effet reel du SSE2.");
        std::puts("std::sort = tri brut par octets (operator<), sans normalisation : reference basse.");
        std::puts("Gains calcules par rapport a 'sans SIMD'.\n");

        for (const Scenario& sc : scenarios)
        {
            Words data = makeData(sc.count, sc.minLen, sc.maxLen, sc.accent);

            auto sortStd   = [&] { Words w = data; std::sort(w.begin(), w.end());    sink = sink + w[0].size(); };
            auto sort      = [&] { Words w = data; SortAlgo::Sort(w);                          sink = sink + w[0].size(); };
            auto sortNaive = [&] { Words w = data; SortAlgoWNormalize::Sort(w);             sink = sink + w[0].size(); };
            auto sortSimd  = [&] { Words w = data; SIMDSortAlgoWNormalize::Sort(w);         sink = sink + w[0].size(); };

            auto normNaive = [&] { size_t a = 0; for (auto& s : data) a += SortAlgoWNormalize::Normalize(s).size();     sink = sink + a; };
            auto normSimd  = [&] { size_t a = 0; for (auto& s : data) a += SIMDSortAlgoWNormalize::Normalize(s).size(); sink = sink + a; };

            std::printf("=== %s | N=%zu ===\n", sc.name, sc.count);
            std::printf("%-18s | %-10s | %-10s | %-10s | %s\n", "", "std::sort", "sans SIMD","Norm sans SIMD", "Norm avec SIMD", "gain SIMD");
            std::puts("-------------------+------------+------------+----------------+----------------+----------");

            double cX = cold(sortStd), cY = cold(sort), cN = cold(sortNaive), cS = cold(sortSimd);
            std::printf("%-18s | %8.2f   | %8.2f   | %8.2f       | %8.2f       | x%.2f\n", "Tri   (froid)", cX, cY, cN, cS, cX / cY);

            double cNn = cold(normNaive), cNs = cold(normSimd);
            std::printf("%-18s | %10s | %10s | %8.2f       | %8.2f       | %s\n", "Normalize (froid)", "-", "-", cNn, cNs, "-");

            double hX = hot(sortStd, 2, 5), hY = hot(sort, 2, 5), hN = hot(sortNaive, 2, 5), hS = hot(sortSimd, 2, 5);
            std::printf("%-18s | %8.2f   | %8.2f   | %8.2f       | %8.2f       | x%.2f\n", "Tri   (chaud)", hX, hY, hN, hS, hX / hY);

            double hNn = hot(normNaive, 3, 11), hNs = hot(normSimd, 3, 11);
            std::printf("%-18s | %10s | %10s | %8.2f       | %8.2f       | %s\n\n", "Normalize (chaud)", "-", "-", hNn, hNs, "-");
        }*/
    }
    
    enum class TokenType : int
    {
        // SINGLE CHARACTER
        L_PARENTHESIS, R_PARENTHESIS,
        DOT, COMMA, NEWLINE,
 
        // LITTERALS
        IDENTIFIER, NUMBER, STRING, COMMENT,
    
        // KEYWORDS
        VAR_DECLARATION, FUNC_DECLARATION, 
        SCOPE_START, SCOPE_END, RETURN,
        COMMENT_START, COMMENT_END,
 
        TRUE, FALSE, ADD, SUB, MUL, DIV, ASSIGN, 
    
        END_OF_FILE
    };
    
    inline void RunTest()
    {
        static std::pair<std::string, TokenType> g_tokenKeywords[] = {
            { "keksoz", TokenType::VAR_DECLARATION },
            { "bazar", TokenType::VAR_DECLARATION },

            { "zafer", TokenType::FUNC_DECLARATION },
            { "fonksyon", TokenType::FUNC_DECLARATION },
            { "travay", TokenType::FUNC_DECLARATION },
    
            { "ouver", TokenType::SCOPE_START },
            { "rant", TokenType::SCOPE_START },
            { "lodebi", TokenType::SCOPE_START },
    
            { "fèrm", TokenType::SCOPE_END },
            { "sétou", TokenType::SCOPE_END },
            { "sorti", TokenType::SCOPE_END },
            { "lafin", TokenType::SCOPE_END },
    
            { "ran", TokenType::RETURN },
            { "rovoy", TokenType::RETURN },
            { "donn", TokenType::RETURN },
            { "ala", TokenType::RETURN },

            { "koz", TokenType::COMMENT_START },
            { "finkoz", TokenType::COMMENT_END },
        
            { "vré", TokenType::TRUE },
            { "pafo", TokenType::TRUE },
            { "fo", TokenType::FALSE },
            { "pavré", TokenType::FALSE },
    
            { "èk", TokenType::ADD },
            { "amplis", TokenType::ADD },
            { "plist", TokenType::ADD },
            { "azout", TokenType::ADD },
    
            { "mwin", TokenType::SUB },
            { "rotir", TokenType::SUB },
            { "anlèv", TokenType::SUB },
    
            { "fwa", TokenType::MUL },
            { "miltipli", TokenType::MUL },
    
            { "koup", TokenType::DIV },
            { "partaz", TokenType::DIV },
            { "kasan", TokenType::DIV },
    
            { "idon", TokenType::ASSIGN },
            { "poufèr", TokenType::ASSIGN },
            { "ifé", TokenType::ASSIGN },
            { "lé", TokenType::ASSIGN },
            { "saidonn", TokenType::ASSIGN },
        };
        
        std::unordered_map<std::string, TokenType> g_tokenKeywordMap = {
            { "keksoz", TokenType::VAR_DECLARATION },
            { "bazar", TokenType::VAR_DECLARATION },

            { "zafer", TokenType::FUNC_DECLARATION },
            { "fonksyon", TokenType::FUNC_DECLARATION },
            { "travay", TokenType::FUNC_DECLARATION },
    
            { "ouver", TokenType::SCOPE_START },
            { "rant", TokenType::SCOPE_START },
            { "lodebi", TokenType::SCOPE_START },
    
            { "fèrm", TokenType::SCOPE_END },
            { "sétou", TokenType::SCOPE_END },
            { "sorti", TokenType::SCOPE_END },
            { "lafin", TokenType::SCOPE_END },
    
            { "ran", TokenType::RETURN },
            { "rovoy", TokenType::RETURN },
            { "donn", TokenType::RETURN },
            { "ala", TokenType::RETURN },

            { "koz", TokenType::COMMENT_START },
            { "finkoz", TokenType::COMMENT_END },
        
            { "vré", TokenType::TRUE },
            { "pafo", TokenType::TRUE },
            { "fo", TokenType::FALSE },
            { "pavré", TokenType::FALSE },
    
            { "èk", TokenType::ADD },
            { "amplis", TokenType::ADD },
            { "plist", TokenType::ADD },
            { "azout", TokenType::ADD },
    
            { "mwin", TokenType::SUB },
            { "rotir", TokenType::SUB },
            { "anlèv", TokenType::SUB },
    
            { "fwa", TokenType::MUL },
            { "miltipli", TokenType::MUL },
    
            { "koup", TokenType::DIV },
            { "partaz", TokenType::DIV },
            { "kasan", TokenType::DIV },
    
            { "idon", TokenType::ASSIGN },
            { "poufèr", TokenType::ASSIGN },
            { "ifé", TokenType::ASSIGN },
            { "lé", TokenType::ASSIGN },
            { "saidonn", TokenType::ASSIGN }
        };
    }
    
    inline void RunBenchmarkSearch()
    {
        static std::pair<std::string, TokenType> g_tokenKeywords[] = {
            { "keksoz", TokenType::VAR_DECLARATION },
            { "bazar", TokenType::VAR_DECLARATION },

            { "zafer", TokenType::FUNC_DECLARATION },
            { "fonksyon", TokenType::FUNC_DECLARATION },
            { "travay", TokenType::FUNC_DECLARATION },
    
            { "ouver", TokenType::SCOPE_START },
            { "rant", TokenType::SCOPE_START },
            { "lodebi", TokenType::SCOPE_START },
    
            { "fèrm", TokenType::SCOPE_END },
            { "sétou", TokenType::SCOPE_END },
            { "sorti", TokenType::SCOPE_END },
            { "lafin", TokenType::SCOPE_END },
    
            { "ran", TokenType::RETURN },
            { "rovoy", TokenType::RETURN },
            { "donn", TokenType::RETURN },
            { "ala", TokenType::RETURN },

            { "koz", TokenType::COMMENT_START },
            { "finkoz", TokenType::COMMENT_END },
        
            { "vré", TokenType::TRUE },
            { "pafo", TokenType::TRUE },
            { "fo", TokenType::FALSE },
            { "pavré", TokenType::FALSE },
    
            { "èk", TokenType::ADD },
            { "amplis", TokenType::ADD },
            { "plist", TokenType::ADD },
            { "azout", TokenType::ADD },
    
            { "mwin", TokenType::SUB },
            { "rotir", TokenType::SUB },
            { "anlèv", TokenType::SUB },
    
            { "fwa", TokenType::MUL },
            { "miltipli", TokenType::MUL },
    
            { "koup", TokenType::DIV },
            { "partaz", TokenType::DIV },
            { "kasan", TokenType::DIV },
    
            { "idon", TokenType::ASSIGN },
            { "poufèr", TokenType::ASSIGN },
            { "ifé", TokenType::ASSIGN },
            { "lé", TokenType::ASSIGN },
            { "saidonn", TokenType::ASSIGN },
        };
        
        using Words = std::vector<std::string>;
        using Clock = std::chrono::steady_clock;
        using Pair  = std::decay_t<decltype(g_tokenKeywords[0])>; 
        using Map   = std::unordered_map<std::string, decltype(Pair::second)>;
         
        std::mt19937 rng(42);
        volatile size_t sink = 0;
         
        const size_t keywordCount = sizeof(g_tokenKeywords) / sizeof(g_tokenKeywords[0]);
         
        std::vector<Pair> unsorted(std::begin(g_tokenKeywords), std::end(g_tokenKeywords));
        Map g_tokenKeywordMap(unsorted.begin(), unsorted.end());
        SortAlgo::Sort(g_tokenKeywords, keywordCount);
         
        auto makeQueries = [&](size_t count, int hitPct) {
            Words v;
            v.reserve(count);
            for (size_t i = 0; i < count; ++i)
            {
                if (static_cast<int>(rng() % 100) < hitPct)
                    v.push_back(g_tokenKeywords[rng() % keywordCount].first);
                else
                {
                    std::string w;
                    do
                    {
                        w.clear();
                        size_t len = 3 + rng() % 8;
                        for (size_t k = 0; k < len; ++k)
                            w += static_cast<char>('a' + rng() % 26);
                    } while (g_tokenKeywordMap.contains(w));
                    v.push_back(std::move(w));
                }
            }
            return v;
        };
         
        auto ms = [](Clock::time_point a, Clock::time_point b) {
            return std::chrono::duration<double, std::milli>(b - a).count();
        };
         
        auto cold = [&](auto fn) {
            auto t0 = Clock::now();
            fn();
            return ms(t0, Clock::now());
        };
         
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
         
        struct Scenario { const char* name; size_t count; int hitPct; };
        const Scenario scenarios[] = {
            { "100 % mots-cles   (hits)",         10000000, 100 },
            { "0 % mots-cles     (misses)",       10000000,   0 },
            { "10 % mots-cles    (lexer typique)", 10000000,  10 },
        };
         
        std::puts("Temps en ms pour N recherches (plus petit = mieux).");
        std::puts("'tableau' = SortAlgo::Sort + SearchAlgo::SearchToken sur g_tokenKeywords.");
        std::puts("Gain = unordered_map / tableau : > 1 veut dire que le tableau est plus rapide.\n");
         
        // ------------------------------------------------------------------ construction
        {
            auto buildMap = [&] {
                Map m(unsorted.begin(), unsorted.end());
                sink = sink + m.size();
            };
            auto buildArray = [&] {
                std::vector<Pair> v = unsorted;
                SortAlgo::Sort(v.data(), v.size());
                sink = sink + v[0].first.size();
            };
            // 1000 constructions par mesure chaude, sinon c'est sous la résolution de l'horloge
            auto buildMapBatch   = [&] { for (int i = 0; i < 1000; ++i) buildMap(); };
            auto buildArrayBatch = [&] { for (int i = 0; i < 1000; ++i) buildArray(); };
         
            std::printf("=== Construction (%zu cles), en us par structure ===\n", keywordCount);
            std::printf("%-18s | %-14s | %-14s | %s\n", "", "unordered_map", "copie + tri", "gain");
            std::puts("-------------------+----------------+----------------+--------");
         
            double cM = cold(buildMap) * 1000.0, cA = cold(buildArray) * 1000.0;
            std::printf("%-18s | %10.2f     | %10.2f     | x%.2f\n", "Construction (froid)", cM, cA, cM / cA);
         
            double hM = hot(buildMapBatch, 2, 7), hA = hot(buildArrayBatch, 2, 7);   // ms pour 1000 -> us pour 1
            std::printf("%-18s | %10.2f     | %10.2f     | x%.2f\n\n", "Construction (chaud)", hM, hA, hM / hA);
        }
         
        // ------------------------------------------------------------------ recherche
        for (const Scenario& sc : scenarios)
        {
            Words queries = makeQueries(sc.count, sc.hitPct);
         
            auto searchMap = [&] {
                size_t c = 0;
                for (const auto& w : queries) c += g_tokenKeywordMap.contains(w) ? 1 : 0;
                sink = sink + c;
            };
            auto searchArray = [&] {
                size_t c = 0;
                for (const auto& w : queries) c += SearchAlgo::SearchToken(g_tokenKeywords, keywordCount, w) ? 1 : 0;
                sink = sink + c;
            };
         
            // Hors chrono : les deux méthodes doivent répondre pareil, sinon le temps du tableau ne veut rien dire
            size_t ecarts = 0;
            for (const auto& w : queries)
                if (g_tokenKeywordMap.contains(w) != SearchAlgo::SearchToken(g_tokenKeywords, keywordCount, w))
                    ++ecarts;
         
            std::printf("=== %s | N=%zu | reponses differentes : %zu ===\n", sc.name, sc.count, ecarts);
            std::printf("%-18s | %-14s | %-14s | %s\n", "", "unordered_map", "tableau", "gain");
            std::puts("-------------------+----------------+----------------+--------");
         
            double cM = cold(searchMap), cA = cold(searchArray);
            std::printf("%-18s | %10.2f     | %10.2f     | x%.2f\n", "Recherche (froid)", cM, cA, cM / cA);
         
            double hM = hot(searchMap, 3, 11), hA = hot(searchArray, 3, 11);
            std::printf("%-18s | %10.2f     | %10.2f     | x%.2f\n\n", "Recherche (chaud)", hM, hA, hM / hA);
        }
    }
}

#endif