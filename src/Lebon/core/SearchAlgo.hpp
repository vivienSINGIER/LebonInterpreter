#ifndef SEARCH_ALGO_HPP
#define SEARCH_ALGO_HPP

#include <string>

namespace SearchAlgo
{
    __declspec(noinline) bool SearchToken(std::pair<std::string, TestSortAlgo::TokenType>* tokens, size_t length, std::string const& word)
    {
        size_t lo = 0;
        size_t hi = length;
        
        while (lo < hi)
        {
            size_t mid = lo + (hi - lo) / 2;
            int c = tokens[mid].first.compare(word);
            if (c == 0) 
                return true;
            
            if (c < 0) 
                lo = mid + 1; 
            else 
                hi = mid;
        }
        return false;
    }
}

#endif
