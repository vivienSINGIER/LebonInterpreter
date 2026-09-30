#include "Lexer.h"

#include "core/Error.h"

Lexer::Lexer(std::string const& _input) : m_content(_input)
{
    m_current = 0;
    m_start = 0;
    m_line = 1;
    m_column = 1;
}

std::vector<Token> const& Lexer::GetTokens()
{
    return m_tokens;
}

bool Lexer::IsAtEnd()
{
    return m_current >= m_content.length();
}

char32_t Lexer::Advance()
{
    size_t length;
    char32_t c = DecodeAt(m_current, length);
    m_current += length;
    m_column++;
    return c;
}

char32_t Lexer::Peek()
{
    if (IsAtEnd()) return U'\0';
    
    size_t length;
    return DecodeAt(m_current, length);
}

bool Lexer::Match(char32_t const _expected)
{
    if (IsAtEnd()) return false;
    size_t length;
    if (DecodeAt(m_current, length) != _expected) 
        return false;
    
    m_current += length;
    m_column++;
    return true;
}

void Lexer::Scan()
{
    while (IsAtEnd() == false)
    {
        ScanToken();
    }
}

void Lexer::ScanToken()
{
    m_start = m_current;
    char32_t c = Advance();
    
    if (c == U'\n')
    {
        AddToken(TokenType::NEWLINE);
        m_line++;
        m_column = 1;
        return;
    }
    if (c == U'\0')
    {
        AddToken(TokenType::END_OF_FILE);
        return;
    }
    if (c == U' ')
        return;
    
    for (auto const& token : g_tokenSingleLetters)
    {
        if (c == token.first)
        {
            AddToken(token.second);
            return;
        }
    }
    
    std::string raw = m_content.substr(m_start, m_current - m_start);
    Error e = Error::Lexical(std::string("unknown token '") + raw + "'\n", m_line, m_column);
    ErrorManager::LogError(e);
}

void Lexer::AddToken(TokenType _type)
{
    Token token;
    token.type = _type;
    token.literal = m_content.substr(m_start, m_current - m_start);
    m_tokens.push_back(token);
}

char32_t Lexer::DecodeAt(size_t _pos, size_t& _length)
{
    unsigned char lead = static_cast<unsigned char>(m_content[_pos]);
    char32_t c;
    
    if (lead < 0x80)                { _length = 1; return lead; }
    if ((lead >> 5) == 0x06)        { _length = 2; c = lead & 0x1F; }
    else if ((lead >> 4) == 0x0E)   { _length = 3; c = lead & 0x0F; }
    else if ((lead >> 3) == 0x1E)   { _length = 4; c = lead & 0x07; }
    else                            { _length = 1; return 0xFFFD; }
    
    if (_pos + _length > m_content.length())
    {
        _length = m_content.length() - _pos;
        return 0xFFFD;
    }
    
    for (size_t i = 1; i < _length; i++)
    {
        unsigned char cont = static_cast<unsigned char>(m_content[_pos + i]);
        if ((cont >> 6) != 0x02)
        {
            _length = i;
            return 0xFFFD;
        }
        c = (c << 6) | (cont & 0x3F);
    }
    return c;
}

bool Lexer::IsIdentifierStart(char32_t _char)
{
    return (_char >= U'A' && _char <= U'Z') || (_char >= 'a' && _char <= 'z') || _char == U'_' || _char >= 0x80;
}

bool Lexer::IsIdentifierPart(char32_t _char)
{
    return IsIdentifierStart(_char) || (_char >= U'0' && _char <= U'9');
}
