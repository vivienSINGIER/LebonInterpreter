#ifndef SORT_ALGO_HPP
#define SORT_ALGO_HPP

#include <algorithm>
#include <ostream>
#include <string>
#include <vector>
#include <smmintrin.h> 

#if defined(_MSC_VER)
    #include <intrin.h>
#endif

static inline unsigned ctz32(unsigned x) 
{
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward(&idx, x);
    return (unsigned)idx;
#else
    return (unsigned)__builtin_ctz(x);
#endif
}

static const char* REPLI[64] = {
    "a","a","a","a","a","a","ae","c","e","e","e","e","i","i","i","i",
    "d","n","o","o","o","o","o","","o","u","u","u","u","y","th","ss",
    "a","a","a","a","a","a","ae","c","e","e","e","e","i","i","i","i",
    "d","n","o","o","o","o","o","","o","u","u","u","u","y","th","y"
};

namespace SortAlgo
{
    struct Element
    {
        uint64_t prefixe;  
        uint32_t offset;   
        uint32_t idx;       
    };
    
    inline std::string BuildKeys(const std::string& mot)
    {
        std::string cle;
        for (size_t i = 0; i < mot.size(); ++i)
        {
            unsigned char c = mot[i];

            if (c == 0xC3 && i + 1 < mot.size())         
            {
                unsigned char d = mot[++i];
                if (d >= 0x80 && d <= 0xBF) cle += REPLI[d - 0x80];
                else { cle += static_cast<char>(c); cle += static_cast<char>(d); } 
            }
            else if (c == 0xC5 && i + 1 < mot.size()
                     && static_cast<unsigned char>(mot[i + 1]) >= 0x92
                     && static_cast<unsigned char>(mot[i + 1]) <= 0x93) 
            {
                ++i;
                cle += "oe";
            }
            else if (c >= 'A' && c <= 'Z')
                cle += static_cast<char>(c + 32);                  
            else
                cle += static_cast<char>(c);
        }

        cle.resize((cle.size() / 16 + 1) * 16, '\0');
        return cle;
    }

    inline int CompareSIMDKeys(const uint8_t* a, const uint8_t* b)
    {
        for (size_t i = 0; ; i += 16)
        {
            __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
            __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));

            unsigned egal = _mm_movemask_epi8(_mm_cmpeq_epi8(va, vb));

            if (egal != 0xFFFF)                             
            {
                unsigned pos = ctz32(~egal);     
                return a[i + pos] < b[i + pos] ? -1 : 1;    
            }

            unsigned zeros = _mm_movemask_epi8(_mm_cmpeq_epi8(va, _mm_setzero_si128()));
            if (zeros) return 0;                           
        }
    }

    struct Comparateur
    {
        const uint8_t*            arene;
        const std::vector<std::string>* mots;

        bool operator()(const Element& a, const Element& b) const
        {
            if (a.prefixe != b.prefixe)
                return a.prefixe < b.prefixe;                  

            int r = CompareSIMDKeys(arene + a.offset, arene + b.offset);
            if (r != 0) return r < 0;

            return (*mots)[a.idx] < (*mots)[b.idx];                
        }
    };
    
    inline void Swap(std::vector<Element>& tab, int a, int b)
    {
        Element temp = tab[a];
        tab[a] = tab[b];
        tab[b] = temp;
    }

    template <class Cmp>
    void Sift(std::vector<Element>& tab, size_t start, size_t node, size_t n, Cmp inf)
    {
        while (true)
        {
            size_t child = 2 * node + 1;
            if (child >= n) break;

            if (child + 1 < n && inf(tab[start + child], tab[start + child + 1]))
                ++child;

            if (!inf(tab[start + node], tab[start + child]))
                break;

            swap(tab, start + node, start + child);
            node = child;
        }
    }

    inline uint64_t PrefixOf(const std::string& cle) 
    {
        uint64_t p = 0;
        for (int i = 0; i < 8; ++i)
            p = (p << 8) | static_cast<unsigned char>(cle[i]);
        return p;
    }
    
    inline void InsertionSort(std::vector<std::string>& tab, size_t start, size_t end)
    {
        for (size_t i = start + 1; i < end; ++i)
        {
            std::string x = tab[i];
            size_t j = i;
            while (j > start && x < tab[j - 1])
            {
                tab[j] = tab[j - 1];
                --j;
            }
            tab[j] = x;
        }
    }
    
    inline void WordsSort(std::vector<std::string>& mots)
    {
        std::vector<uint8_t> arene;
        std::vector<Element> elems;
        elems.reserve(mots.size());

        for (uint32_t i = 0; i < mots.size(); ++i)
        {
            std::string cle = BuildKeys(mots[i]);
            elems.push_back({ PrefixOf(cle), static_cast<uint32_t>(arene.size()), i });
            arene.insert(arene.end(), cle.begin(), cle.end());
        }

        Comparateur inf{ arene.data(), &mots };  

        std::sort(elems.begin(), elems.end(), inf);

        std::vector<std::string> res;
        res.reserve(mots.size());
        for (const Element& e : elems) res.push_back(std::move(mots[e.idx]));
        mots = std::move(res);
    }
}

#endif