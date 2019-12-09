#ifndef TOKEN_H
#define TOKEN_H

#include <string>

std::string const IF = "IF";  // if
std::string const ELSE = "ELSE";  // else
std::string const FOR = "FOR";  // for
std::string const WHILE = "WHILE";  // while
std::string const DO = "DO";  // do
std::string const RETURN = "RETURN";  // return
std::string const BREAK = "BREAK";  // break
std::string const CONTINUE = "CONTINUE";  // continue

// Variables
std::string const VAR = "VAR";  // var

// Constants
std::string const NUM = "NUM";  // number

// Types
std::string const STRUCT = "STRUCT";  // struct
std::string const SIGNED = "SIGNED";  // signed
std::string const UNSIGNED = "UNSIGNED";  // unsigned
std::string const LONG = "LONG";  // long
std::string const SHORT = "SHORT";  // short
std::string const INT = "INT";  // int
std::string const FLOAT = "FLOAT";  // float
std::string const DOUBLE = "DOUBLE";  // double
std::string const CHAR = "CHAR";  // char

// Binary operators
std::string const PLUS = "PLUS";  // +
std::string const MINUS = "MINUS";  // -
std::string const ASTERISK = "ASTERISK";  // *
std::string const DIV = "DIV";  // /
std::string const MOD = "MOD";  // %
std::string const BITAND = "BITAND";  // &
std::string const BITOR = "BITOR";  // |
std::string const BITXOR = "BITXOR";  // ^
std::string const LSHIFT = "LSHIFT";  // <<
std::string const RSHIFT = "RSHIFT";  // >>
std::string const GREATER = "GREATER";  // >
std::string const LESSER = "LESSER";  // <
std::string const GREEQ = "GREEQ";  // >=
std::string const LESEQ = "LESEQ";  // <=
std::string const EQ = "EQ";  // ==
std::string const NEQ = "NEQ";  // !=
std::string const AND = "AND";  // &&
std::string const OR = "OR";  // ||

// Unary operators
std::string const NOT = "NOT";  // !
std::string const INC = "INC";  // ++
std::string const DEC = "DEC";  // --
std::string const BITNOT = "BITNOT";  // ~

// Assignment operators
std::string const ASSIGN = "ASSIGN";  // =
std::string const ASSIGNPLUS = "ASSIGNPLUS";  // +=
std::string const ASSIGNMINUS = "ASSIGNMINUS";  // -=
std::string const ASSIGNMUL = "ASSIGNMUL";  // *=
std::string const ASSIGNDIV = "ASSIGNDIV";  // /=
std::string const ASSIGNMOD = "ASSIGNMOD";  // %=
std::string const ASSIGNAND = "ASSIGNAND";  // &=
std::string const ASSIGNOR = "ASSIGNOR";  // |=
std::string const ASSIGNXOR = "ASSIGNXOR";  // ^=
std::string const ASSIGNLSHIFT = "ASSIGNLSHIFT";  // <<=
std::string const ASSIGNRSHIFT = "ASSIGNRSHIFT";  // >>=

// Braces, parentheses and brackets
std::string const LPAREN = "LPAREN";  // (
std::string const RPAREN = "RPAREN";  // )
std::string const LBRACE = "LBRACE";  // {
std::string const RBRACE = "RBRACE";  // }
std::string const LBRACKET = "LBRACKET";  // [
std::string const RBRACKET = "RBRACKET";  // ]

// Ternary operators
std::string const QUESTION = "QUESTION";  // ?
std::string const COLON = "COLON";  // :

// Punctuation
std::string const SEMICOLON = "SEMICOLON";  // ;
std::string const COMA = "COMA";  // ,

// Undefined
std::string const UNDEFINED = "UNDEFINED";

// End of file
std::string const END = "END";  // eof

struct token_t {
    std::string type;
    std::string value;
};

#endif /* ifndef TOKEN_H */
