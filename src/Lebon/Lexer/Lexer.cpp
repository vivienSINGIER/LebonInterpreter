#include "Lexer.h"

#include <iomanip>
#include <iostream>

#include "core/Error.h"

Lexer::Lexer(fs::path const& _path)
{
    Error e = FileHelper::ReadFile(_path, m_content);
    if (e)
        ErrorManager::LogError(e);
    m_current = 0;
    m_start = 0;
    m_line = 1;
    m_column = 1;
}

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

char32_t Lexer::PeekAt(size_t _pos)
{
    if (_pos >= m_content.length())
        return U'\0';
    
    size_t length;
    return DecodeAt(_pos, length);
}

bool Lexer::Match(char32_t _expected)
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

void Lexer::DisplayTokens()
{
    for (auto const& token : m_tokens)
    {
        std::string const& typeName = g_tokenTypeNames[static_cast<int>(token.type)];
        std::string literal = token.type == TokenType::NEWLINE ? "\\n" : token.literal;

        std::cout << std::left
                  << std::setw(14) << typeName
                  << " row " << std::setw(4) << token.row
                  << " col " << std::setw(4) << token.column
                  << " '" << literal << "'\n";
    }
}

void Lexer::Comment()
{
    size_t bodyStart = m_current;
    
    while (!IsAtEnd())
    {
        char32_t c = Peek();
        
        if (c == U'\n')
        {
            Advance();
            m_line++;
            m_column = 1;
            continue;
        }
        
        if (IsIdentifierStart(c))
        {
            size_t wordStart = m_current;
            
            while (IsIdentifierPart(Peek()))
                Advance();
            
            std::string_view word(m_content.data() + wordStart, m_current - wordStart);
            if (word == "finkoz")
            {
                Token token;
                token.type = TokenType::COMMENT;
                token.literal = m_content.substr(bodyStart, wordStart - bodyStart);
                token.row = m_startLine;
                token.column = m_startColumn;
                m_tokens.push_back(token);
                return;
            }
            continue;
        }
        
        Advance();
    }
    
    Error e = Error::Lexical("unterminated comment, missing 'finkoz'\n", m_startLine, m_startColumn);
    ErrorManager::LogError(e);
}

void Lexer::String()
{
    uint32_t startColumn = m_column - 1;

    while (Peek() != U'\"' && Peek() != U'\n' && Peek() != U'\r' && !IsAtEnd())
        Advance();

    if (Peek() != U'\"')
    {
        std::string raw = m_content.substr(m_start, m_current - m_start);
        Error e = Error::Lexical(std::string("unterminated string '") + raw + "'\n", m_line, startColumn);
        ErrorManager::LogError(e);
        return;
    }
    
    Advance();
    Token token;
    token.type = TokenType::STRING;
    token.literal = m_content.substr(m_start + 1, m_current - m_start - 2);
    token.row = m_startLine;
    token.column = m_startColumn;
    m_tokens.push_back(token);
}

void Lexer::Number()
{
    while (IsNumerical(Peek()))
        Advance();
    
    if (Peek() == U'.' && IsNumerical(PeekAt(m_current + 1)))
    {
        Advance();
        while (IsNumerical(Peek()))
            Advance();
    }

    
    AddToken(TokenType::NUMBER);
}

void Lexer::Identifier()
{
    while (IsIdentifierPart(Peek()))
        Advance();
    
    std::string_view word(m_content.data() + m_start, m_current - m_start);
    
    for (auto const& token : g_tokenKeywords)
    {
        if (word == token.first)
        {
            if (token.second == TokenType::COMMENT_START)
            {
                Comment();
                return;
            }
            if (token.second == TokenType::COMMENT_END)
            {
                Error e = Error::Lexical("'finkoz' without matching 'koz'\n", m_startLine, m_startColumn);
                ErrorManager::LogError(e);
                return;
            }
            
            AddToken(token.second);
            return;
        }
    }
    
    AddToken(TokenType::IDENTIFIER);
}

void Lexer::ScanToken()
{
    m_start = m_current;
    m_startLine = m_line;
    m_startColumn = m_column;
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
    if (c == U' ' || c == U'\r' || c == U'\t')
        return;
    
    for (auto const& token : g_tokenSingleLetters)
    {
        if (c == token.first)
        {
            AddToken(token.second);
            return;
        }
    }
    
    if (c == U'\"')
    {
        String();
        return;
    }
    
    if (IsIdentifierStart(c))
    {
        Identifier();
        return;
    }
    
    if (IsNumerical(c))
    {
        Number();
        return;
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
    token.row = m_startLine;
    token.column = m_startColumn;
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

bool Lexer::IsAlphabetical(char32_t _char)
{
    if ( (_char >= U'a' && _char <= U'z') || (_char >= U'A' && _char <= U'Z') )
        return true;
    
    if (_char >= 0x00C0 && _char <= 0x00FF)                                                                                                                                                                                       
        return _char != 0x00D7 && _char != 0x00F7; 
    
    return false;
}

bool Lexer::IsNumerical(char32_t _char)
{
    return (_char >= U'0' && _char <= U'9');
}

bool Lexer::IsIdentifierStart(char32_t _char)
{
    unsigned char uc = static_cast<unsigned char>(_char);
    return IsAlphabetical(uc) || uc == U'_' || uc >= 0x80;
}

bool Lexer::IsIdentifierPart(char32_t _char)
{
    unsigned char uc = static_cast<unsigned char>(_char);
    return IsIdentifierStart(uc) || IsNumerical(uc) || uc >= 0x80;
}
