#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>

#include "token.h"
#include "node.h"

class Parser {
private:
    std::vector<token_t> lexer;
    token_t *current_token;
    int lexer_pos;

    token_t *getNextToken();
    void eat(std::string token_type);
    void error(std::string type_expected, std::string type_actual);

    node_t *factor();
    node_t *term();
    node_t *expression();
    node_t *variable();
    node_t *empty();
    node_t *type();
    std::pair<node_t*, node_t*> typeVar();
    node_t *declarationList();
    node_t *declaration();
    node_t *assignment();
    node_t *returnStatement();
    node_t *statement();
    node_t *ifStatement();
    node_t *ternaryStatement();
    node_t *whileStatement();
    node_t *doWhileStatement();
    node_t *forStatement();
    node_t *statementList();
    node_t *compoundStatement();
    node_t *arrayIndex();
    node_t *number();
    node_t *functionDeclaration();
    node_t *functionCall();
    node_t *functionCallArguments();
    node_t *argument();
    node_t *argumentList();
    node_t *programStatementList();
    node_t *program();

public:
    Parser(std::vector<token_t> lexer);
    ~Parser();

    node_t *parse();
};

#endif /* ifndef PARSER_H */
