#ifndef LEXER_H
#define LEXER_H

#include "token.h"

#include <vector>
#include <string>

class Lexer {
private:
    std::vector<std::string> reservedWords;
    std::vector<std::string> operators;
    std::vector<std::string> symbols;

public:
    Lexer();
    ~Lexer();

    void setReservedWords(std::vector<std::string> const&);
    void setOperators(std::vector<std::string> const&);
    void setSymbols(std::vector<std::string> const&);

    bool isVariable(std::string const&) const;
    bool isNumber(std::string const&) const;

    std::vector<std::string> splitByToken(std::string const&, std::string const&, bool) const;
    std::vector<std::string> splitByTokens(std::string const&, std::vector<std::string> const&, bool) const;

    token_t toToken(std::string const&) const;

    std::vector<token_t> lex(std::string const&) const;
};

#endif /* ifndef LEXER_H */
