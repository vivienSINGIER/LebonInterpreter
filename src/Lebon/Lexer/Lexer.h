#ifndef LEXER_H_DEFINED
#define LEXER_H_DEFINED

#include <string>
#include <vector>

#include "Tokens.hpp"


class Lexer
{
public:
    Lexer(std::string const& input);
    
    std::vector<Token> const& GetTokens();
    
    bool IsAtEnd();
    char Advance();
    
    void ScanToken();
    void AddToken(TokenType _type);
    
private:
    std::string const m_content; 
    std::vector<Token> m_tokens;
    
    uint64_t m_current;
    uint64_t m_start;
    uint64_t m_line;
    uint64_t m_column;
};

#endif
