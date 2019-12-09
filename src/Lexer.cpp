#include "Lexer.h"

#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include "token.h"

Lexer::Lexer() {
    reservedWords = std::vector<std::string> {
        "if",
        "for",
        "else",
        "while",
        "do",
        "return",
        "switch",
        "case",
        "auto",
        "break",
        "continue",
        "default",
        "goto",
        "extern",
        "int",
        "float",
        "short",
        "char",
        "sin",
        "cos",
        "tan",
        "pow"
    };

    operators = std::vector<std::string> {
        ">=",
        "<<=",
        "==",
        "!=",
        "--",
        "++",
        "+=",
        "-=",
        "*=",
        "/=",
        "&=",
        "|=",
        "^=",
        "&&",
        "||",
        "<<",
        ">>",
        "&",
        "|",
        "=",
        "+",
        "-",
        "*",
        "/",
        "(",
        ")",
        ">",
        "<",
        "[",
        "]",
        "%",
        "!",
        "^",
        "~"
    };

    symbols = std::vector<std::string> {
        ";",
        "{",
        "}",
        ","
    };
}

Lexer::~Lexer() = default;

void Lexer::setReservedWords(std::vector<std::string> const &reservedWords) {
    this->reservedWords = reservedWords;
}

void Lexer::setOperators(std::vector<std::string> const &operators) {
    this->operators = operators;
}

void Lexer::setSymbols(std::vector<std::string> const &symbols) {
    this->symbols = symbols;
}

bool Lexer::isVariable(std::string const &word) const {
    std::regex variable("([a-zA-Z_])+([a-zA-Z_0-9])*");
    return std::regex_match(word, variable);
}

bool Lexer::isNumber(std::string const &word) const {
    std::regex number("((\\+|-)?[[:digit:]]+)(\\.(([[:digit:]]+)?))?((e|E)((\\+|-)?)[[:digit:]]+)?");
    return std::regex_match(word, number);
}

std::vector<std::string> Lexer::splitByToken(std::string const &word, std::string const &token, bool fullWord) const {
    std::vector<std::string> result;

    std::string token_copy;

    switch (token.size()) {
        case 1:
            token_copy = "\\" + token;
            break;
        case 2:
            // That works. It's a problem.
            token_copy = std::string("\\") + token[0] + std::string("\\") + token[1];
            break;
        case 3:
            token_copy = std::string("\\") + token[0] + std::string("\\") + token[1] + std::string("\\") + token[2];
            break;
        default:
            token_copy = token;
            break;
    }
    if (fullWord) {
        token_copy =  "\\b" + token_copy + "\\b";
    }
    std::regex regex(token_copy);

    std::smatch match;
    std::regex_search(word, match, regex);

    if (!match.size()) {
        result.push_back(word);
        return result;
    }

    /* auto span = std::make_pair(match.position(), match.position() + match.length()); */
    auto beforeToken = word.substr(0, match.position());
    if (beforeToken.size()) {
        for (auto const &sub_token : splitByToken(beforeToken, token, fullWord)) {
            result.push_back(sub_token);
        }
    }

    result.push_back(token);

    auto afterToken = word.substr(match.position() + match.length(), std::string::npos);
    if (afterToken.size()) {
        for (auto const &sub_token : splitByToken(afterToken, token, fullWord)) {
            result.push_back(sub_token);
        }
    }

    return result;
}

std::vector<std::string> Lexer::splitByTokens(std::string const &text, std::vector<std::string> const &tokens, bool fullWord) const {
    std::vector<std::string> result{text};

    for (auto const &token : tokens) {
        std::vector<std::string> tmp_result;
        for (auto const &sublist : result) {
            for (auto const &item : splitByToken(sublist, token, fullWord)) {
                tmp_result.push_back(item);
            }
        }
        result = tmp_result;
    }

    size_t i = 0;
    while (i < result.size() - 1) {
        auto combined = result.at(i) + result.at(i + 1);
        if (std::find(operators.begin(), operators.end(), combined) != operators.end()) {
            result.at(i) = combined;
            result.erase(result.begin() + i + 1);
        }
        i++;
    }

    return result;
}

std::vector<token_t> Lexer::lex(std::string const &text) const {
    std::vector<token_t> result;

    auto new_text = text;
    new_text.erase(remove(new_text.begin(), new_text.end(), '\n'), new_text.end());
    new_text.erase(remove(new_text.begin(), new_text.end(), '\t'), new_text.end());
    new_text.erase(remove(new_text.begin(), new_text.end(), '\r'), new_text.end());

    std::vector<std::string> symbols_and_operators;
    symbols_and_operators.reserve(symbols.size() + operators.size() + 1);

    symbols_and_operators.insert(symbols_and_operators.end(), symbols.begin(), symbols.end());
    symbols_and_operators.insert(symbols_and_operators.end(), operators.begin(), operators.end());
    symbols_and_operators.push_back(" ");

    auto lexemes = splitByTokens(new_text, symbols_and_operators, false);
    size_t count = 0;
    while (count < lexemes.size()) {
        if (lexemes.at(count) == " ") {
            lexemes.erase(lexemes.begin() + count--);
        }
        ++count;
    }

    for (size_t i = 0; i < lexemes.size(); ++i) {
        result.push_back(this->toToken(lexemes.at(i)));
    }
    std::vector<std::string> stack;
    for (int i = 0; i < lexemes.size(); ++i) {
        if (lexemes.at(i) == "{") {
            stack.push_back("{");
        } else if (lexemes.at(i) == "}") {
            if (stack.empty()) {
                throw std::logic_error("Syntax error. Unbalanced braces at token " + std::to_string(i));
            }
            stack.pop_back();
        }
    }
    std::vector<std::string> stack2;
    for (int i = 0; i < lexemes.size(); ++i) {
        if (lexemes.at(i) == "(") {
            stack2.push_back("(");
        } else if (lexemes.at(i) == ")") {
            if (stack2.empty()) {
                throw std::logic_error("Syntax error. Unbalanced parentheses at token " + std::to_string(i));
            }
            stack2.pop_back();
        }
    }
    std::vector<std::string> stack3;
    for (int i = 0; i < lexemes.size(); ++i) {
        if (lexemes.at(i) == "[") {
            stack3.push_back("[");
        } else if (lexemes.at(i) == "]") {
            if (stack3.empty()) {
                throw std::logic_error("Syntax error. Unbalanced brackets at token " + std::to_string(i));
            }
            stack3.pop_back();
        }
    }

    return result;
}

token_t Lexer::toToken(std::string const &lexeme) const {
    if (lexeme == "if") {
        auto token = token_t();
        token.type = IF;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "else") {
        auto token = token_t();
        token.type = ELSE;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "for") {
        auto token = token_t();
        token.type = FOR;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "while") {
        auto token = token_t();
        token.type = WHILE;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "do") {
        auto token = token_t();
        token.type = DO;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "return") {
        auto token = token_t();
        token.type = RETURN;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "break") {
        auto token = token_t();
        token.type = BREAK;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "continue") {
        auto token = token_t();
        token.type = CONTINUE;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "struct") {
        auto token = token_t();
        token.type = STRUCT;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "signed") {
        auto token = token_t();
        token.type = SIGNED;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "unsigned") {
        auto token = token_t();
        token.type = UNSIGNED;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "long") {
        auto token = token_t();
        token.type = LONG;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "short") {
        auto token = token_t();
        token.type = SHORT;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "int") {
        auto token = token_t();
        token.type = INT;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "float") {
        auto token = token_t();
        token.type = FLOAT;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "double") {
        auto token = token_t();
        token.type = DOUBLE;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "char") {
        auto token = token_t();
        token.type = CHAR;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "+") {
        auto token = token_t();
        token.type = PLUS;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "-") {
        auto token = token_t();
        token.type = MINUS;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "*") {
        auto token = token_t();
        token.type = ASTERISK;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "/") {
        auto token = token_t();
        token.type = DIV;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "%") {
        auto token = token_t();
        token.type = MOD;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "&") {
        auto token = token_t();
        token.type = BITAND;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "|") {
        auto token = token_t();
        token.type = BITOR;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "^") {
        auto token = token_t();
        token.type = BITXOR;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "<<") {
        auto token = token_t();
        token.type = LSHIFT;
        token.value = lexeme;
        return token;
    }
    if (lexeme == ">>") {
        auto token = token_t();
        token.type = RSHIFT;
        token.value = lexeme;
        return token;
    }
    if (lexeme == ">") {
        auto token = token_t();
        token.type = GREATER;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "<") {
        auto token = token_t();
        token.type = LESSER;
        token.value = lexeme;
        return token;
    }
    if (lexeme == ">=") {
        auto token = token_t();
        token.type = GREEQ;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "<=") {
        auto token = token_t();
        token.type = LESEQ;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "==") {
        auto token = token_t();
        token.type = EQ;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "!=") {
        auto token = token_t();
        token.type = NEQ;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "&&") {
        auto token = token_t();
        token.type = AND;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "||") {
        auto token = token_t();
        token.type = OR;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "!") {
        auto token = token_t();
        token.type = NOT;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "++") {
        auto token = token_t();
        token.type = INC;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "--") {
        auto token = token_t();
        token.type = DEC;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "~") {
        auto token = token_t();
        token.type = BITNOT;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "=") {
        auto token = token_t();
        token.type = ASSIGN;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "+=") {
        auto token = token_t();
        token.type = ASSIGNPLUS;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "-=") {
        auto token = token_t();
        token.type = ASSIGNMINUS;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "*=") {
        auto token = token_t();
        token.type = ASSIGNMUL;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "/=") {
        auto token = token_t();
        token.type = ASSIGNDIV;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "%=") {
        auto token = token_t();
        token.type = ASSIGNMOD;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "&=") {
        auto token = token_t();
        token.type = ASSIGNAND;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "|=") {
        auto token = token_t();
        token.type = ASSIGNOR;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "^=") {
        auto token = token_t();
        token.type = ASSIGNXOR;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "<<=") {
        auto token = token_t();
        token.type = ASSIGNLSHIFT;
        token.value = lexeme;
        return token;
    }
    if (lexeme == ">>=") {
        auto token = token_t();
        token.type = ASSIGNRSHIFT;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "(") {
        auto token = token_t();
        token.type = LPAREN;
        token.value = lexeme;
        return token;
    }
    if (lexeme == ")") {
        auto token = token_t();
        token.type = RPAREN;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "[") {
        auto token = token_t();
        token.type = LBRACKET;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "]") {
        auto token = token_t();
        token.type = RBRACKET;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "{") {
        auto token = token_t();
        token.type = LBRACE;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "}") {
        auto token = token_t();
        token.type = RBRACE;
        token.value = lexeme;
        return token;
    }
    if (lexeme == "?") {
        auto token = token_t();
        token.type = QUESTION;
        token.value = lexeme;
        return token;
    }
    if (lexeme == ":") {
        auto token = token_t();
        token.type = COLON;
        token.value = lexeme;
        return token;
    }
    if (lexeme == ";") {
        auto token = token_t();
        token.type = SEMICOLON;
        token.value = lexeme;
        return token;
    }
    if (lexeme == ",") {
        auto token = token_t();
        token.type = COMA;
        token.value = lexeme;
        return token;
    }
    if (this->isNumber(lexeme)) {
        auto token = token_t();
        token.type = NUM;
        token.value = lexeme;
        return token;
    }
    if (this->isVariable(lexeme)) {
        auto token = token_t();
        token.type = VAR;
        token.value = lexeme;
        return token;
    }
    auto token = token_t();
    token.type = UNDEFINED;
    token.value = lexeme;
    return token;
}

