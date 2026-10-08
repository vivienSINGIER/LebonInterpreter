#ifndef LEXER_H_DEFINED
#define LEXER_H_DEFINED

#include <string>
#include <vector>

#include "Tokens.hpp"

#include "../core/FileHelper.h"

class Lexer
{
public:
    Lexer(fs::path const& _path);
    Lexer(std::string const& input);
    
    std::vector<Token> const& GetTokens();
    
    void Scan();
    void DisplayTokens();
    
private:
    std::string m_content; 
    std::vector<Token> m_tokens;
    
    bool m_isCommented = false;
    
    size_t m_current;
    size_t m_start;
    
    uint32_t m_startLine;
    uint32_t m_startColumn;
    uint32_t m_line;
    uint32_t m_column;
    
    void Comment();
    void String();
    void Number();
    void Identifier();
    void ExtendHyphenKeyword();
    void ScanToken();
    void AddToken(TokenType _type);
    
    bool IsAtEnd();
    char32_t Advance();
    char32_t Peek();
    char32_t PeekAt(size_t _pos);
    bool Match(char32_t _expected);
    
    char32_t DecodeAt(size_t _pos, size_t& _length);
    
    bool IsAlphabetical(char32_t _char);
    bool IsNumerical(char32_t _char);
    bool IsIdentifierStart(char32_t _char);
    bool IsIdentifierPart(char32_t _char);
};

#endif
