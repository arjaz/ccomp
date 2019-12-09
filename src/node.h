#ifndef NODE_H
#define NODE_H

#include <vector>
#include <string>

#include "token.h"

std::string const PROGRAM_NODE = "PROGRAM";
std::string const STATEMENT_NODE = "STATEMENT";
std::string const STATEMENT_LIST_NODE = "STATEMENT_LIST";
std::string const COMPOUND_NODE = "COMPOUND_STATEMENT";
std::string const FUNCTION_DECLARATION_NODE = "FUNCTION_DECLARATION";
std::string const FUNCTION_CALL_NODE = "FUNCTION_CALL";
std::string const FUNCTION_CALL_ARGUMENTS_NODE = "FUNCTION_CALL_ARGUMENTS";
std::string const ARGUMENT_NODE = "ARGUMENT";
std::string const ARGUMENT_LIST_NODE = "ARGUMENT_LIST";
std::string const ARRAY_INDEX_NODE = "ARRAY_INDEX";
std::string const IF_NODE = "IF_STATEMENT";
std::string const TERNARY_NODE = "TERNARY";
std::string const FOR_NODE = "FOR_STATEMENT";
std::string const WHILE_NODE = "WHILE_STATEMENT";
std::string const DO_WHILE_NODE = "DO_WHILE";
std::string const ASSIGNMENT_NODE = "ASSIGNMENT";
std::string const BIN_OP_NODE = "BIN_OP";
std::string const UN_OP_NODE = "UN_OP";
std::string const VAR_NODE = "VAR";
std::string const NUM_NODE = "NUM";
std::string const VARIABLE_DECLARATION_NODE = "VARIABLE_DECLARATION";
std::string const VARIABLE_DECLARATION_LIST_NODE = "VARIABLE_DECLARATION_LIST";
std::string const TYPE_NODE = "TYPE";
std::string const CONTINUE_NODE = "CONTINUE";
std::string const BREAK_NODE = "BREAK";
std::string const RETURN_NODE = "RETURN";
std::string const EMPTY_NODE = "EMPTY";
std::string const POINTER_NODE = "POINTER";

struct node_t {
    std::vector<node_t*> children{};
    token_t token{};
    std::string type{};
};

#endif /* ifndef NODE_H */
