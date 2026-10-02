#ifndef SEARCH_ALGO_HPP
#define SEARCH_ALGO_HPP

#include <string>

namespace SearchAlgo
{
    __declspec(noinline) bool SearchToken(std::pair<std::string, TestSortAlgo::TokenType>* tokens, size_t length, std::string const& word, int middle)
    {
        if ( middle <= 0)
            return false;
        
        if (tokens[middle].first == word)
            return true;
        
        if ( tokens[middle].first < word)
            middle += middle / 2;
        else
            middle -= middle / 2;
        
        SearchToken(tokens, length, word, middle);
    }
}

#endif
