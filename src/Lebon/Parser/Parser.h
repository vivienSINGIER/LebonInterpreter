#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "../Lexer/Tokens.hpp"

class Parser
{
public:

private:
	std::vector<Token> vTokens;
	void Peek();
	void Match();
};

#endif // !PARSER_H_INCLUDED