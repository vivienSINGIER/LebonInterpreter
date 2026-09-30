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
    
    void Scan();
    
private:
    std::string const m_content; 
    std::vector<Token> m_tokens;
    
    size_t m_current;
    size_t m_start;
    uint32_t m_line;
    uint32_t m_column;
    
    void ScanToken();
    void AddToken(TokenType _type);
    
    bool IsAtEnd();
    char32_t Advance();
    char32_t Peek();
    bool Match(char32_t const _expected);
    
    char32_t DecodeAt(size_t _pos, size_t& _length);
    
    bool IsIdentifierStart(char32_t _char);
    bool IsIdentifierPart(char32_t _char);
};

#endif
