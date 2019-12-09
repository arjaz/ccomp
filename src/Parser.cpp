#include "Parser.h"

#include "token.h"
#include "node.h"

#include <vector>
#include <stdexcept>

#include <iostream>

Parser::Parser(std::vector<token_t> lexer) : lexer(lexer), lexer_pos(0) {
    this->current_token = this->getNextToken();
}
Parser::~Parser() = default;

void Parser::error(std::string type_expected, std::string type_actual) {
    std::string error_msg = ""
        "Syntax error.\n"
        "Expected " + type_expected + ".\n"
        "Got " + type_actual + " instead.\n"
        "Token to the left is " + lexer.at(lexer_pos - 2).type + "\n"
        "Token is " + lexer.at(lexer_pos - 1).type + "\n"
        "Token to the right is " + lexer.at(lexer_pos).type + "\n";
    throw std::logic_error(error_msg);
}

token_t *Parser::getNextToken() {
    if (this->lexer_pos == this->lexer.size()) {
        static auto token = new token_t();
        token->type = END;
        token->value = "END";
        return token;
    }
    auto token = &(this->lexer.at(this->lexer_pos++));
    return token;
}

void Parser::eat(std::string token_type) {
    /* std::cout << "Eating " << token_type << " but having " << this->current_token->type <<  std::endl; */
    if (this->current_token->type == token_type) {
        this->current_token = this->getNextToken();
    } else {
        this->error(token_type, this->current_token->type);
    }
}

// UN_OP factor
// | NUM
// | LPAREN expression RPAREN
// | variable (INC | DEC)*
// | functionCall
// | arrayIndex
node_t *Parser::factor() {
    auto token = *(this->current_token);
    if (
            this->current_token->type == PLUS ||
            this->current_token->type == MINUS ||
            this->current_token->type == NOT ||
            this->current_token->type == BITNOT ||
            this->current_token->type == INC ||
            this->current_token->type == DEC ||
            this->current_token->type == ASTERISK
        ) {
        this->eat(this->current_token->type);
        auto node = new node_t();
        node->type = UN_OP_NODE;
        node->token = token;
        node->children.push_back(this->factor());
        return node;
    } else if (this->current_token->type == NUM) {
        auto node = this->number();
        return node;
    } else if (this->current_token->type == LPAREN) {
        this->eat(LPAREN);
        auto node = this->expression();
        this->eat(RPAREN);
        return node;
    } else if (this->current_token->type == VAR) {
        this->eat(VAR);
        if (this->current_token->type == LBRACKET) {
            this->lexer_pos -= 2;
            this->current_token = this->getNextToken();
            auto node = this->arrayIndex();
            return node;
        } else if (this->current_token->type == LPAREN) {
            this->lexer_pos -= 2;
            this->current_token = this->getNextToken();
            auto node = this->functionCall();
            return node;
        } else if (this->current_token->type == INC || this->current_token->type == DEC) {
            this->lexer_pos -= 2;
            this->current_token = this->getNextToken();
            auto var = new node_t();
            var->type = VAR_NODE;
            var->token = *(this->current_token);
            this->eat(VAR);
            auto node = new node_t();
            node->type = UN_OP_NODE;
            node->token = *(this->current_token);
            this->eat(this->current_token->type);
            node->children.push_back(var);
            return node;
        }
        this->lexer_pos -= 2;
        this->current_token = this->getNextToken();
        return this->variable();
    }
    this->error("proper factor", "something else");
}

// factor ((ASTERISK | DIV | MOD) factor)*
node_t *Parser::term() {
    auto node = this->factor();

    while (
            this->current_token->type == ASTERISK ||
            this->current_token->type == DIV ||
            this->current_token->type == MOD
          ) {
        auto token = *(this->current_token);
        this->eat(token.type);

        // FIXME: this ain't gonna work
        // btw, it somehow does
        auto node_tmp = node;
        auto node_new = new node_t();
        node_new->type = BIN_OP_NODE;
        node_new->children.push_back(node_tmp);
        node_new->token = token;
        node_new->children.push_back(this->factor());
        node = node_new;
    }

    return node;
}

// term ((PLUS | MINUS | EQ | NEQ | AND | OR | BITAND | BITOR | BITXOR) term)*
node_t *Parser::expression() {
    auto node = this->term();

    while (
            this->current_token->type == PLUS ||
            this->current_token->type == MINUS ||
            this->current_token->type == AND ||
            this->current_token->type == OR ||
            this->current_token->type == BITAND ||
            this->current_token->type == BITOR ||
            this->current_token->type == BITXOR ||
            this->current_token->type == EQ ||
            this->current_token->type == NEQ
          ) {
        auto token = *(this->current_token);
        this->eat(token.type);
        auto node_new = new node_t();
        node_new->type = BIN_OP_NODE;
        node_new->children.push_back(node);
        node_new->token = token;
        node_new->children.push_back(this->term());
        node = node_new;
    }

    return node;
}

// VAR LBRACKET expression RBRACKET
node_t *Parser::arrayIndex() {
    /* std::cout << "arrayIndex()" << std::endl; */
    auto node = new node_t();
    node->type = ARRAY_INDEX_NODE;
    /* auto var = this->variable(); */
    auto var = new node_t();
    var->token = *(this->current_token);
    var->type = VAR_NODE;
    this->eat(VAR);
    this->eat(LBRACKET);
    auto index = this->expression();
    this->eat(RBRACKET);
    node->children.push_back(var);
    node->children.push_back(index);
    return node;
}

// variable LPAREN functionCallArguments RPAREN
node_t *Parser::functionCall() {
    auto node = new node_t();
    node->type = FUNCTION_CALL_NODE;
    auto var = this->variable();
    this->eat(LPAREN);
    auto arguments = this->functionCallArguments();
    this->eat(RPAREN);
    node->children.push_back(var);
    node->children.push_back(arguments);
    return node;
}

// empty
// expression
// expression COMA functionCallArguments
node_t *Parser::functionCallArguments() {
    auto node = new node_t();
    node->type = FUNCTION_CALL_ARGUMENTS_NODE;
    if (this->current_token->type == RPAREN)
        return node;
    node->children.push_back(this->expression());
    while (this->current_token->type == COMA) {
        this->eat(COMA);
        node->children.push_back(this->expression());
    }
    return node;
}

// (ASTERISK)* VAR (LBRACKET NUM RBRACKET)*
node_t *Parser::variable() {
    /* std::cout << "variable() is called" << std::endl; */
    auto node = new node_t();
    node->type = VAR_NODE;
    bool pointer{false};
    bool array{false};
    if (this->current_token->type == ASTERISK) {
        this->eat(ASTERISK);
        pointer = true;
    }
    node->token = *(this->current_token);
    this->eat(VAR);
    if (this->current_token->type == LBRACKET) {
        this->eat(LBRACKET);
        auto size = new node_t();
        size->type = NUM_NODE;
        size->token = *current_token;
        this->eat(NUM);
        this->eat(RBRACKET);
        array = true;
        node->children.push_back(size);
    }
    if (pointer) {
        auto pointer = new node_t();
        pointer->type = POINTER_NODE;
        node->children.push_back(pointer);
    }
    if (array) {
        auto pointer = new node_t();
        pointer->type = POINTER_NODE;
        node->children.push_back(pointer);
    }
    return node;
}

// NUM
node_t *Parser::number() {
    auto node = new node_t();
    node->type = NUM_NODE;
    node->token = *(this->current_token);
    this->eat(NUM);
    return node;
}

node_t *Parser::empty() {
    auto node = new node_t();
    node->type = EMPTY_NODE;
    return node;
}

// (INT | FLOAT | DOUBLE | CHAR | LONG | SHORT | UNSIGNED)
node_t *Parser::type() {
    /* std::cout << "type() is called" << std::endl; */
    auto token = *(this->current_token);
    if (
            token.type == INT ||
            token.type == FLOAT ||
            token.type == DOUBLE ||
            token.type == CHAR ||
            token.type == LONG ||
            token.type == SHORT ||
            token.type == UNSIGNED
       ) {
        auto node = new node_t();
        node->type = TYPE_NODE;
        node->token = token;
        this->eat(this->current_token->type);
        return node;
    }
    this->error("type name", this->current_token->type);
}

// type variable
std::pair<node_t*, node_t*> Parser::typeVar() {
    /* std::cout << "typeVar() is called" << std::endl; */
    auto type_node = this->type();
    auto var_node = this->variable();
    return std::make_pair(type_node, var_node);
}

// variable (ASSIGN expression)*
node_t *Parser::declaration() {
    /* std::cout << "declaration() is called" << std::endl; */
    auto var = this->variable();
    if (this->current_token->type == ASSIGN) {
        this->eat(ASSIGN);
        auto expr = this->expression();
        auto node = new node_t();
        node->type = VARIABLE_DECLARATION_NODE;
        node->children.push_back(var);
        node->children.push_back(expr);
        return node;
    } else {
        auto node = new node_t();
        node->type = VARIABLE_DECLARATION_NODE;
        node->children.push_back(var);
        return node;
    }
}

// variable ASSIGNMENT_OP expression
node_t *Parser::assignment() {
    auto left = this->variable();
    auto token = *(this->current_token);
    if (token.type == ASSIGN) {
        this->eat(token.type);
    } else {
        this->error("assignment operator", this->current_token->type);
    }
    auto right = this->expression();
    auto node = new node_t();
    node->type = ASSIGNMENT_NODE;
    node->children.push_back(left);
    node->children.push_back(right);
    node->token = token;
    return node;
}

// RETURN expression
// RETURN
node_t *Parser::returnStatement() {
    this->eat(RETURN);
    if (this->current_token->type == SEMICOLON) {
        auto node = new node_t();
        node->type = RETURN_NODE;
        return node;
    } else {
        auto expr = this->expression();
        auto node = new node_t();
        node->type = RETURN_NODE;
        node->children.push_back(expr);
        return node;
    }
}

// type declaration (COMA declaration)+
node_t *Parser::declarationList() {
    auto node = new node_t();
    node->type = VARIABLE_DECLARATION_LIST_NODE;
    auto type = this->type();
    auto decl = this->declaration();
    node->children.push_back(type);
    node->children.push_back(decl);
    while (this->current_token->type == COMA) {
        this->eat(COMA);
        decl = this->declaration();
        node->children.push_back(decl);
    }
    return node;
}

// assignment SEMICOLON
// | declaration_list SEMICOLON
// | returnStatement SEMICOLON
// | (INC | DEC) variable SEMICOLON
// | compoundStatement
// | functionDeclaration
// | ifStatement
// | whileStatement
// | forStatement
// | CONTINUE SEMICOLON
// | BREAK SEMICOLON
// | returnStatement SEMICOLON
// | empty
node_t *Parser::statement() {
    /* std::cout << "statement()" << std::endl; */
    if (this->current_token->type == LBRACE) {
        auto node = this->compoundStatement();
        return node;
    } else if (
            this->current_token->type == INT ||
            this->current_token->type == FLOAT ||
            this->current_token->type == DOUBLE ||
            this->current_token->type == CHAR ||
            this->current_token->type == SHORT ||
            this->current_token->type == LONG ||
            this->current_token->type == UNSIGNED
            ) {
        this->eat(this->current_token->type);
        int offset = 0;
        if (this->current_token->type == ASTERISK) {
            this->eat(ASTERISK);
            offset = 1;
        }
        this->eat(VAR);
        if (this->current_token->type == LPAREN) {
            this->lexer_pos -= 3 + offset;
            this->current_token = this->getNextToken();
            auto node = this->functionDeclaration();
            return node;
        }
        this->lexer_pos -= 3 + offset;
        this->current_token = this->getNextToken();
        auto node = this->declarationList();
        this->eat(SEMICOLON);
        return node;
    } else if (this->current_token->type == VAR) {
        this->eat(VAR);
        if (this->current_token->type == INC || this->current_token->type == DEC) {
            this->lexer_pos -= 2;
            this->current_token = this->getNextToken();
            auto node = this->expression();
            this->eat(SEMICOLON);
            return node;
        }
        this->lexer_pos -= 2;
        this->current_token = this->getNextToken();
        auto node = this->assignment();
        this->eat(SEMICOLON);
        return node;
    } else if (this->current_token->type == RETURN) {
        auto node = this->returnStatement();
        this->eat(SEMICOLON);
        return node;
    } else if (this->current_token->type == WHILE) {
        auto node = this->whileStatement();
        return node;
    } else if (this->current_token->type == FOR) {
        auto node = this->forStatement();
        return node;
    } else if (this->current_token->type == IF) {
        auto node = this->ifStatement();
        return node;
    } else if (this->current_token->type == CONTINUE) {
        this->eat(CONTINUE);
        auto node = new node_t();
        node->type = CONTINUE_NODE;
        this->eat(SEMICOLON);
        return node;
    } else if (this->current_token->type == BREAK) {
        this->eat(BREAK);
        auto node = new node_t();
        node->type = BREAK_NODE;
        this->eat(SEMICOLON);
        return node;
    } else if (this->current_token->type == INC || this->current_token->type == DEC) {
        auto node = this->expression();
        this->eat(SEMICOLON);
        return node;
    } else {
        auto node = this->empty();
        return node;
    }
}

// IF LPAREN expression RPAREN statement
// | IF LPAREN expression RPAREN statement ELSE statement
node_t *Parser::ifStatement() {
    this->eat(IF);
    this->eat(LPAREN);
    auto if_condition = this->expression();
    this->eat(RPAREN);
    auto if_statement = this->statement();

    if (this->current_token->type == ELSE) {
        this->eat(ELSE);
        auto else_statement = this->statement();
        auto node = new node_t();
        node->type = IF_NODE;
        node->children.push_back(if_condition);
        node->children.push_back(if_statement);
        node->children.push_back(else_statement);
        return node;
    } else {
        auto node = new node_t();
        node->type = IF_NODE;
        node->children.push_back(if_condition);
        node->children.push_back(if_statement);
        return node;
    }
}

// expression QUESTION expression COLON expression
node_t *Parser::ternaryStatement() {
    auto if_condition = this->expression();
    this->eat(QUESTION);
    auto if_expr = this->expression();
    this->eat(COLON);
    auto else_expr = this->expression();
    auto node = new node_t();
    node->type = TERNARY_NODE;
    node->children.push_back(if_condition);
    node->children.push_back(if_expr);
    node->children.push_back(else_expr);
    return node;
}

// WHILE LPAREN expression RPAREN statement
node_t *Parser::whileStatement() {
    this->eat(WHILE);
    this->eat(LPAREN);
    auto condition = this->expression();
    this->eat(RPAREN);
    auto statement = this->statement();
    auto node = new node_t();
    node->type = WHILE_NODE;
    node->children.push_back(condition);
    node->children.push_back(statement);
    return node;
}

// DO statement WHILE LPAREN expression RPAREN SEMICOLON
node_t *Parser::doWhileStatement() {
    this->eat(DO);
    auto statement = this->statement();
    this->eat(WHILE);
    this->eat(LPAREN);
    auto condition = this->expression();
    this->eat(RPAREN);
    this->eat(SEMICOLON);
    auto node = new node_t();
    node->type = DO_WHILE_NODE;
    node->children.push_back(statement);
    node->children.push_back(condition);
    return node;
}

// FOR LPAREN expression SEMICOLON expression SEMICOLON expression RPAREN STATEMENT
node_t *Parser::forStatement() {
    this->eat(FOR);
    this->eat(LPAREN);
    auto loop_init = this->expression();
    this->eat(SEMICOLON);
    auto loop_condition = this->expression();
    this->eat(SEMICOLON);
    auto loop_post_expression = this->expression();
    this->eat(RPAREN);
    auto loop_body = this->statement();
    auto node = new node_t();
    node->type = FOR_NODE;
    node->children.push_back(loop_init);
    node->children.push_back(loop_condition);
    node->children.push_back(loop_post_expression);
    node->children.push_back(loop_body);
    return node;
}

// statement
// | statement statementList
node_t *Parser::statementList() {
    auto node_result = new node_t();
    node_result->type = STATEMENT_LIST_NODE;

    auto node = this->statement();
    node_result->children.push_back(node);
    while (
            this->current_token->type == LBRACE ||
            this->current_token->type == VAR ||
            this->current_token->type == RETURN ||
            this->current_token->type == INT ||
            this->current_token->type == FLOAT ||
            this->current_token->type == DOUBLE ||
            this->current_token->type == CHAR ||
            this->current_token->type == SHORT ||
            this->current_token->type == LONG ||
            this->current_token->type == UNSIGNED ||
            this->current_token->type == DEC ||
            this->current_token->type == INC ||
            this->current_token->type == IF ||
            this->current_token->type == WHILE ||
            this->current_token->type == DO ||
            this->current_token->type == CONTINUE ||
            this->current_token->type == BREAK
          )
    {
        auto new_node = this->statement();
        node_result->children.push_back(new_node);
    }
    if (this->current_token->type != END && this->current_token->type != RBRACE) {
        this->error(RBRACE, this->current_token->type);
    }

    return node_result;
}

// LBRACE statementList RBRACE
node_t *Parser::compoundStatement() {
    this->eat(LBRACE);
    auto node = new node_t();
    node->type = COMPOUND_NODE;
    node->children.push_back(this->statementList());
    this->eat(RBRACE);
    return node;
}

// typeVar LPAREN argumentList RPAREN LBRACE statementList RBRACE
node_t *Parser::functionDeclaration() {
    /* std::cout << "functionDeclaration() is called" << std::endl; */
    auto [func_type, func_name] = this->typeVar();
    this->eat(LPAREN);
    auto func_arguments = this->argumentList();
    this->eat(RPAREN);
    this->eat(LBRACE);
    auto func_body = this->statementList();
    this->eat(RBRACE);
    auto node = new node_t();
    node->type = FUNCTION_DECLARATION_NODE;
    node->children.push_back(func_type);
    node->children.push_back(func_name);
    node->children.push_back(func_arguments);
    node->children.push_back(func_body);
    return node;
}

// empty
// argument
// argument COMA argumentList
node_t *Parser::argumentList() {
    auto node = new node_t();
    node->type = ARGUMENT_LIST_NODE;
    if (this->current_token->type == RPAREN) {
        return node;
    }
    node->children.push_back(this->argument());
    while (this->current_token->type != RPAREN) {
        this->eat(COMA);
        node->children.push_back(this->argument());
    }
    return node;
}

// typeVar
node_t *Parser::argument() {
    auto [argument_type, argument_name] = this->typeVar();
    auto node = new node_t();
    node->type = ARGUMENT_NODE;
    node->children.push_back(argument_type);
    node->children.push_back(argument_name);
    return node;
}

// statement END
// statement programStatementList
node_t *Parser::programStatementList() {
    auto node = new node_t();
    node->type = STATEMENT_LIST_NODE;
    while (this->current_token->type != END) {
        node->children.push_back(this->statement());
    }
    return node;
}

node_t *Parser::program() {
    auto node = new node_t();
    node->type = PROGRAM_NODE;
    node->children.push_back(this->statementList());
    return node;
}

node_t *Parser::parse() {
    return this->program();
}
