#ifndef SORT_ALGO_HPP
#define SORT_ALGO_HPP

#include <iostream>
#include <string>
#include <vector>
#include <emmintrin.h>

namespace SortAlgo
{
    inline std::string Normalize(const std::string& s)
    {
        std::string r;
        r.reserve(s.size());

        for (size_t i = 0; i < s.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(s[i]);

            if (c == 0xC3 && i + 1 < s.size())
            {
                unsigned char d = static_cast<unsigned char>(s[i + 1]);
                
                if (d >= 0x80 && d <= 0x9E && d != 0x97)
                    d += 0x20;

                char base = 0;
                if      (d >= 0xA0 && d <= 0xA5) base = 'a'; 
                else if (d == 0xA7)              base = 'c'; 
                else if (d >= 0xA8 && d <= 0xAB) base = 'e'; 
                else if (d >= 0xAC && d <= 0xAF) base = 'i'; 
                else if (d == 0xB1)              base = 'n'; 
                else if (d >= 0xB2 && d <= 0xB6) base = 'o'; 
                else if (d >= 0xB9 && d <= 0xBC) base = 'u'; 
                else if (d == 0xBD || d == 0xBF) base = 'y'; 

                if (base != 0)
                {
                    r += base;
                    ++i;
                    continue;
                }
            }

            r += static_cast<char>(std::tolower(c));
        }
        return r;
    }
    
    inline bool Less(const std::string& a, const std::string& b)
    {
        std::string na = Normalize(a);
        std::string nb = Normalize(b);

        if (na != nb)
            return na < nb;

        return a > b;
    }
    
    inline void Swap(std::vector<std::string>& tab, int a, int b)
    {
        std::string temp = tab[a];
        tab[a] = tab[b];
        tab[b] = temp;
    }
    
    inline void Sift(std::vector<std::string>& tab, int start, int node, int n)
    {
        int k = node;
        int j = 2 * k;
        while (j <= n)
        {
            if ( j < n && Less(tab[start + j - 1], tab[start + j]))
                j++;
            
            if (Less(tab[start + k - 1], tab[start + j - 1]))
            {
                Swap(tab, start + k - 1, start + j - 1);
                k = j;
                j = 2 * k;
            }
            else
            {
                j = n + 1;
            }
        }
    }
    
    inline void Heapsort(std::vector<std::string>& tab, int start, int end)
    {
        int lenght = end - start + 1;
        
        for (int i = lenght / 2; i >= 1; --i)
            Sift(tab, start, i, lenght);
        
        for (int i = lenght; i >= 2; --i)
        {
            Swap(tab, start + i - 1, start);
            Sift(tab, start, 1, i - 1);
        }
    }
    
    inline void IntroSort(std::vector<std::string>& tab, int depthLimit, int min, int max)
    {
        if (min >= max) return;
        if (depthLimit <= 0)
        {
            Heapsort(tab, min, max);
            return;
        }

        int i = min, j = max;
        std::string pivot = tab[(min + max) / 2];

        while (i <= j)
        {
            while (Less(tab[i], pivot)) ++i;
            while (Less(pivot, tab[j])) --j;
            if (i <= j)
            {
                Swap(tab, i, j);
                ++i;
                --j;
            }
        }

        IntroSort(tab, depthLimit - 1, min, j);
        IntroSort(tab, depthLimit - 1, i, max);
    }
    
    inline void Sort(std::vector<std::string>& mots)
    {
        int n = static_cast<int>(mots.size());
        if (n < 2) return;

        int log2n = 0;
        for (int m = n; m > 1; m >>= 1) log2n++;

        IntroSort(mots, 2 * log2n, 0, n - 1);
    }
    
    inline void Print(const std::vector<std::string>& tab)
    {
        std::cout << "| ";
        for (const std::string& s : tab)
        {
            std::cout << s << " | ";
        }
        std::cout << '\n';
    }
}

namespace SIMDSortAlgo
{
    struct Entry
    {
        std::string word;    
        std::string key;     
        uint64_t    prefix;   
    };

    
    inline size_t NormalizeChar(const std::string& s, size_t i, char& out)
    {
        unsigned char c = static_cast<unsigned char>(s[i]);

        if (c == 0xC3 && i + 1 < s.size())
        {
            unsigned char d = static_cast<unsigned char>(s[i + 1]);
            if (d >= 0x80 && d <= 0x9E && d != 0x97)
                d += 0x20;

            char base = 0;
            if      (d >= 0xA0 && d <= 0xA5) base = 'a';
            else if (d == 0xA7)              base = 'c';
            else if (d >= 0xA8 && d <= 0xAB) base = 'e';
            else if (d >= 0xAC && d <= 0xAF) base = 'i';
            else if (d == 0xB1)              base = 'n';
            else if (d >= 0xB2 && d <= 0xB6) base = 'o';
            else if (d >= 0xB9 && d <= 0xBC) base = 'u';
            else if (d == 0xBD || d == 0xBF) base = 'y';

            if (base != 0) { out = base; return 2; }
        }

        if (c >= 'A' && c <= 'Z') c += 0x20;
        out = static_cast<char>(c);
        return 1;
    }

    inline std::string Normalize(const std::string& s)
    {
        const size_t n = s.size();
        std::string r(n, '\0');         
        size_t i = 0, o = 0;

        const __m128i lo  = _mm_set1_epi8('A' - 1);
        const __m128i hi  = _mm_set1_epi8('Z' + 1);
        const __m128i bit = _mm_set1_epi8(0x20);

        while (i < n)
        {
            if (i + 16 <= n)
            {
                __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s.data() + i));

                if (_mm_movemask_epi8(v) == 0)
                {
                    __m128i isUpper = _mm_and_si128(_mm_cmpgt_epi8(v, lo),
                                                    _mm_cmplt_epi8(v, hi));
                    v = _mm_or_si128(v, _mm_and_si128(isUpper, bit));
                    _mm_storeu_si128(reinterpret_cast<__m128i*>(&r[o]), v);
                    i += 16;
                    o += 16;
                    continue;
                }
            }
            char c;
            i += NormalizeChar(s, i, c);
            r[o++] = c;
        }

        r.resize(o);
        return r;
    }

    inline uint64_t PrefixOf(const std::string& key)
    {
        uint64_t p = 0;
        for (size_t i = 0; i < 8; ++i)
            p = (p << 8) | (i < key.size() ? static_cast<unsigned char>(key[i]) : 0);
        return p;
    }

    inline bool Less(const Entry& a, const Entry& b)
    {
        if (a.prefix != b.prefix)
            return a.prefix < b.prefix;

        int c = a.key.compare(b.key);
        if (c != 0)
            return c < 0;

        return a.word > b.word;       
    }

    inline void Swap(std::vector<Entry>& tab, int a, int b)
    {
        Entry temp = tab[a];
        tab[a] = tab[b];
        tab[b] = temp;
    }

    inline void Sift(std::vector<Entry>& tab, int start, int node, int n)
    {
        int k = node;
        int j = 2 * k;
        while (j <= n)
        {
            if (j < n && Less(tab[start + j - 1], tab[start + j]))
                j++;

            if (Less(tab[start + k - 1], tab[start + j - 1]))
            {
                Swap(tab, start + k - 1, start + j - 1);
                k = j;
                j = 2 * k;
            }
            else
            {
                j = n + 1;
            }
        }
    }

    inline void Heapsort(std::vector<Entry>& tab, int start, int end)
    {
        int length = end - start + 1;

        for (int i = length / 2; i >= 1; --i)
            Sift(tab, start, i, length);

        for (int i = length; i >= 2; --i)
        {
            Swap(tab, start + i - 1, start);
            Sift(tab, start, 1, i - 1);
        }
    }

    inline void IntroSort(std::vector<Entry>& tab, int depthLimit, int min, int max)
    {
        if (min >= max) return;
        if (depthLimit <= 0)
        {
            Heapsort(tab, min, max);
            return;
        }

        int i = min, j = max;
        Entry pivot = tab[(min + max) / 2];

        while (i <= j)
        {
            while (Less(tab[i], pivot)) ++i;
            while (Less(pivot, tab[j])) --j;
            if (i <= j)
            {
                Swap(tab, i, j);
                ++i;
                --j;
            }
        }

        IntroSort(tab, depthLimit - 1, min, j);
        IntroSort(tab, depthLimit - 1, i, max);
    }

    inline void Sort(std::vector<std::string>& mots)
    {
        int n = static_cast<int>(mots.size());
        if (n < 2) return;

        std::vector<Entry> entries;
        entries.reserve(n);
        for (std::string& w : mots)
        {
            std::string key = Normalize(w);
            uint64_t p = PrefixOf(key);
            entries.push_back({ std::move(w), std::move(key), p });
        }

        int log2n = 0;
        for (int m = n; m > 1; m >>= 1) log2n++;
        IntroSort(entries, 2 * log2n, 0, n - 1);

        for (int i = 0; i < n; ++i)
            mots[i] = std::move(entries[i].word);
    }

    inline void Print(const std::vector<std::string>& tab)
    {
        std::cout << "| ";
        for (const std::string& s : tab)
            std::cout << s << " | ";
        std::cout << '\n';
    }
}

#endif