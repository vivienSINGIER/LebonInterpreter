#ifndef TOKENS_HPP_DEFINED
#define TOKENS_HPP_DEFINED

#include <string>
#include <string_view>
#include <vector>

enum TokenType : int
{
    // SINGLE CHARACTER
    QUOTE, DOUBLE_QUOTE, L_PARENTHESIS, R_PARENTHESIS,
    DOT, COMMA, NEWLINE,
 
    // LITTERALS
    IDENTIFIER, NUMBER, STRING,
    
    // KEYWORDS
    VAR_DECLARATION, FUNC_DECLARATION, 
    SCOPE_START, SCOPE_END, RETURN,
    COMMENT_START, COMMENT_END,
 
    TRUE, FALSE, ADD, SUB, MUL, DIV, ASSIGN,
};

struct Token
{
    TokenType type;
    std::string literal;
    int row, column;
};

static std::pair<std::string, TokenType> const tokenSpecs[] = {
    { "'", TokenType::QUOTE },
    { "\"", TokenType::DOUBLE_QUOTE },
    { "(", TokenType::L_PARENTHESIS },
    { ")", TokenType::R_PARENTHESIS },
    { ".", TokenType::DOT },
    { ",", TokenType::COMMA },
    {"\n", TokenType::NEWLINE },
    
    { "keksoz", TokenType::VAR_DECLARATION },
    { "bazar", TokenType::VAR_DECLARATION },

    { "zafer", TokenType::FUNC_DECLARATION },
    { "fonksyon", TokenType::FUNC_DECLARATION },
    { "travay", TokenType::FUNC_DECLARATION },
    
    { "ouver", TokenType::SCOPE_START },
    { "rant", TokenType::SCOPE_START },
    { "lodebi", TokenType::SCOPE_START },
    
    { "fèrm", TokenType::SCOPE_END },
    { "sétou", TokenType::SCOPE_END },
    { "sorti", TokenType::SCOPE_END },
    { "lafin", TokenType::SCOPE_END },
    
    { "ran", TokenType::RETURN },
    { "rovoy", TokenType::RETURN },
    { "donn", TokenType::RETURN },
    { "ala", TokenType::RETURN },

    { "koz", TokenType::COMMENT_START },
    { "finkoz", TokenType::COMMENT_END },
        
    { "vré", TokenType::TRUE },
    { "pafo", TokenType::TRUE },
    { "fo", TokenType::FALSE },
    { "pavré", TokenType::FALSE },
    
    { "èk", TokenType::ADD },
    { "amplis", TokenType::ADD },
    { "plist", TokenType::ADD },
    { "azout", TokenType::ADD },
    
    { "mwin", TokenType::SUB },
    { "rotir", TokenType::SUB },
    { "anlèv", TokenType::SUB },
    
    { "fwa", TokenType::MUL },
    { "miltipli", TokenType::MUL },
    
    { "koup", TokenType::DIV },
    { "partaz", TokenType::DIV },
    { "kasan", TokenType::DIV },
    
    { "idon", TokenType::ASSIGN },
    { "poufèr", TokenType::ASSIGN },
    { "ifé", TokenType::ASSIGN },
    { "lé", TokenType::ASSIGN },
    { "saidonn", TokenType::ASSIGN },
};

#endif
