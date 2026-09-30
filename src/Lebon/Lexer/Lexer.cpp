#include "Lexer.h"

Lexer::Lexer(std::string const& _input) : m_content(_input)
{
    m_current = 0;
    m_start = 0;
    m_line = 1;
    
}

std::vector<Token> const& Lexer::GetTokens()
{
    return m_tokens;
}

bool Lexer::IsAtEnd()
{
    return m_current >= m_content.length();
}

char Lexer::Advance()
{
    return m_content[m_current++];
}

void Lexer::ScanToken()
{
    char c = Advance();
    
}

void Lexer::AddToken(TokenType _type)
{
    Token token;
    token.type = _type;
    token.literal = m_content.substr(m_start, m_current - m_start);
}
