#include "Visiter.h"

#include "node.h"

#include <iostream>
#include <exception>

Visiter::Visiter() = default;
Visiter::~Visiter() = default;

variable_data::variable_data(std::string name, int size, std::string type, std::string value, int stack_offset, int array_size) :
    name(name), size(size), type(type), value(value), stack_offset(stack_offset), array_size(array_size) {}

void Visiter::visualize(node_t *node, int depth) {
    std::cout << "|";
    for (int i = 0; i < depth + 1; ++i) {
        std::cout << "--";
    }
    std::cout << "Depth: " << depth << " " << node->type << " " << node->token.value << std::endl;
    std::cout << "|";
    for (int i = 0; i < depth + 1; ++i) {
        std::cout << "  ";
    }
    std::cout << "It has " << node->children.size() << " children." << std::endl;

    for (auto const &x : node->children) {
        Visiter::visualize(x, depth + 1);
    }
}

void Visiter::visitNodes(node_t *node, node_t *parent) {
    if (node->type == VARIABLE_DECLARATION_NODE) {
        variableDeclaration(node, parent);
    } else if (node->type == FUNCTION_DECLARATION_NODE) {
        functionDeclaration(node, parent);
    } else if (node->type == VAR_NODE) {
        variable(node, parent);
    } else if (node->type == BIN_OP_NODE) {
        binaryExpression(node, parent);
    } else if (node->type == ASSIGNMENT_NODE) {
        assignment(node, parent);
    } else if (node->type == ARRAY_INDEX_NODE) {
        arrayIndex(node, parent);
    } else if (node->type == FUNCTION_CALL_NODE) {
        functionCall(node, parent);
    } else if (node->type == UN_OP_NODE) {
        unaryExpression(node, parent);
    }
    for (int i = 0; i < node->children.size(); ++i) {
        this->visitNodes(node->children.at(i), node);
    }
}

void Visiter::variable(node_t *node, node_t *parent) {
    //std::cout << "variable()" << std::endl;
    /* if (variables.count(node->token.value) == 0 && parent->type != FUNCTION_DECLARATION_NODE && parent->type != FUNCTION_CALL_NODE && parent->type != ARGUMENT_NODE) { */
    /*     throw std::logic_error("Variable " + node->token.value + " is used but not defined"); */
    /* } */
    if (!variablePresent(node->token.value) && parent->type != FUNCTION_DECLARATION_NODE && parent->type != FUNCTION_CALL_NODE && parent->type != ARGUMENT_NODE)
        throw std::logic_error("Variable " + node->token.value + " is used but not defined");
}

bool Visiter::variablePresent(std::string name) {
    for (int i = 0; i < variables.size(); ++i) {
        if (variables.at(i)->name == name)
            return true;
    }
    return false;
}

void Visiter::variableDeclaration(node_t *node, node_t *parent) {
    //std::cout << "variableDeclaration()" << std::endl;
    if (variablePresent(node->children.at(0)->token.value) + functions.count(node->children.at(0)->token.value) == 0) {
        std::string value = "0";
        if (node->children.size() >= 2) {
            value = node->children.at(1)->token.value;
        }
        variables.push_back(std::make_unique<variable_data>(
                node->children.at(0)->token.value,
                // TODO: size
                4,
                parent->children.at(0)->token.value,
                value,
                0,
                0
        ));
        for (auto const &x : node->children.at(0)->children) {
            if (x->type == POINTER_NODE) {
                variables.back()->type += "*";
            } else if (x->type == NUM_NODE) {
                if (!isPositiveInteger(x->token.value)) {
                    throw std::logic_error("Array size must be positive integer at variable " + node->children.at(0)->token.value + " declaration");
                }
                variables.back()->array_size = std::stoi(x->token.value);
            }
        }
    } else {
        throw std::logic_error("Variable " + node->children.at(0)->token.value + " is declared more than once");
    }
}

void Visiter::functionDeclaration(node_t *node, node_t *parent) {
    //std::cout << "functionDeclaration()" << std::endl;
    if (
            variablePresent(node->children.at(1)->token.value) == 0 &&
            functions.count(node->children.at(1)->token.value) == 0
        ) {
        functions[node->children.at(1)->token.value] = node->children.at(0)->token.value;
    } else {
        throw std::logic_error("Function " + node->children.at(1)->token.value + " is declared more than once");
    }
}

void Visiter::functionCall(node_t *node, node_t *parent) {
    //std::cout << "functionCall()" << std::endl;
    if (functions.count(node->children.at(0)->token.value) == 0) {
        functions[node->children.at(0)->token.value] = "int";
    }
}

void Visiter::binaryExpression(node_t *node, node_t *parent) {
    //std::cout << "binaryExpression()" << std::endl;
    auto type_left = this->typeOf(node->children.at(0));
    auto type_right = this->typeOf(node->children.at(1));

    if (isPointer(type_left) && isPointer(type_right) && node->token.type == PLUS) {
        throw std::logic_error("Unsupported binary operator " + node->token.value + " for pointers at " + nodeToString(node));
    }

    if ((isPointer(type_left) || isPointer(type_right)) && (node->token.type != PLUS && node->token.type != MINUS && node->token.type != AND && node->token.type != OR)) {
        throw std::logic_error("Unsupported binary operator " + node->token.value + " for pointers at " + nodeToString(node));
    }
}

void Visiter::unaryExpression(node_t *node, node_t *parent) {
    //std::cout << "unaryExpression()" << std::endl;
    auto type = typeOf(node);
    if (isPointer(type) && node->token.type != ASTERISK && node->token.type != INC && node->token.type != DEC) {
        throw std::logic_error("Pointer in unary expression is not allowed at " + nodeToString(node));
    }
}

void Visiter::assignment(node_t *node, node_t *parent) {
    //std::cout << "assignment()" << std::endl;
    auto type_left = this->typeOf(node->children.at(0));
    auto type_right = this->typeOf(node->children.at(1));

    if (isPointer(type_left) && !isPointer(type_right) && (type_right == "float" || type_right == "double" || type_right == "char")) {
        throw std::logic_error("Unsupported type conversion at " + nodeToString(node));
    }

    if (!isPointer(type_left) && isPointer(type_right) && (type_left == "float" || type_left == "double")) {
        throw std::logic_error("Unsupported type conversion at " + nodeToString(node));
    }
}

void Visiter::arrayIndex(node_t *node, node_t *parent) {
    //std::cout << "arrayIndex()" << std::endl;
    auto type = typeOf(node->children.at(1));
    if (type != "int" && type != "unsigned" && type != "long" && type != "short" && type != "char") {
        throw std::logic_error("The index of an array is not of integer type at " + nodeToString(node) + ", child of " + nodeToString(parent));
    }
}

std::string Visiter::typeOf(node_t *node) {
    //std::cout << "typeOf()" << std::endl;
    if (node->type == VAR_NODE) {
        if (!variablePresent(node->token.value)) {
            throw std::logic_error("Variable " + nodeToString(node) + " is used but not defined.");
        }
        for (int i = 0; i < variables.size(); ++i) {
            if (variables.at(i)->name == node->token.value) {
                return variables.at(i)->type;
            }
        }
    }
    if (node->type == UN_OP_NODE) {
        if (node->token.type == ASTERISK) {
            if (isPointer(typeOf(node->children.at(0)))) {
                auto type = typeOf(node->children.at(0));
                std::string res(std::begin(type), std::end(type) - 1);
                return res;
            }
            throw std::logic_error("Pointer expected at " + nodeToString(node));
        }
        return this->typeOf(node->children.at(0));
    }
    if (node->type == BIN_OP_NODE) {
        if (node->token.type == AND || node->token.type == OR || node->token.type == EQ || node->token.type == NEQ || node->token.type == GREEQ || node->token.type == LESEQ || node->token.type == GREATER || node->token.type == LESSER) {
            return "char";
        }
        return this->biggerType(this->typeOf(node->children.at(0)), this->typeOf(node->children.at(1)));
    }
    if (node->type == NUM_NODE) {
        return isInteger(node->token.value) ? "int" : "float";
    }
    if (node->type == ARRAY_INDEX_NODE) {
        return stripPointer(this->typeOf(node->children.at(0)), 1);
    }
    if (node->type == FUNCTION_CALL_NODE) {
        return "int";
    }
    throw std::logic_error("Unable to detect a type at " + nodeToString(node));
}

// TODO: type compatibility
bool Visiter::areCompatible(std::string type1, std::string type2) {
    return true;
}

std::string Visiter::stripPointer(std::string type, int depth) {
    if (depth == 0) {
        std::string res;
        for (int i = 0; i < type.size(); ++i) {
            if (type.at(i) == '*')
                break;
            res += type.at(i);
        }
        return res;
    }
    std::string res(std::begin(type), std::end(type) - depth);

    return res;
}

// TODO: type deduction
std::string Visiter::biggerType(std::string type1, std::string type2) {
    if (isPointer(type1) && !isPointer(type2))
        return type1;
    if (isPointer(type2) && !isPointer(type1))
        return type2;

    if (isPointer(type1) && isPointer(type2))
        return biggerType(stripPointer(type1, 0), stripPointer(type2, 0));

    if (type1 == "double" || type2 == "double")
        return "double";

    if (type1 == "float" || type2 == "float")
        return "float";

    if (type1 == "long" || type2 == "long")
        return "long";

    if (type1 == "int" || type2 == "int")
        return "int";

    if (type1 == "unsigned" || type2 == "unsigned")
        return "unsigned";

    if (type1 == "short" || type2 == "short")
        return "short";

    return type1;
}

void Visiter::printVariables() {
    for (auto const &x : variables) {
        std::cout << "Variable " << x->name << " of type " << x->type << " of size " << x->size << " with value " << x->value << std::endl;
        std::cout << (x->array_size ? "It is an array of size " + std::to_string(x->array_size) : "") << std::endl;
    }
    // for (auto const &x : functions) {
        //std::cout << x.first << " returning type " << x.second << std::endl;
    // }
}

bool Visiter::isPointer(std::string type) {
    return type.back() == '*';
}

bool Visiter::isInteger(std::string value) {
    if (value.empty() || ((!isdigit(value[0]) && (value[0] != '-') && (value[0] != '+'))))
        return false;
    char *p;
    strtol(value.c_str(), &p, 10);

    return (*p == 0);
}

bool Visiter::isPositiveInteger(std::string value) {
    return isInteger(value) && value[0] != '-';
}

std::string Visiter::nodeToString(node_t *node) {
    if (node->type == ARRAY_INDEX_NODE) {
        return nodeToString(node->children.at(0)) + "[" + nodeToString(node->children.at(1)) + "]";
    }
    if (node->type == VAR_NODE) {
        return node->token.value;
    }
    if (node->type == UN_OP_NODE) {
        return node->token.value + nodeToString(node->children.at(0));
    }
    if (node->type == BIN_OP_NODE) {
        return nodeToString(node->children.at(0)) + node->token.value + nodeToString(node->children.at(1));
    }
    if (node->type == ASSIGNMENT_NODE) {
        return nodeToString(node->children.at(0)) + node->token.value + nodeToString(node->children.at(1));
    }
    if (node->token.value != "")
        return node->token.value;
    std::string res;
    for (auto const &x : node->children) {
        res += nodeToString(x);
    }
    return res;
}

std::vector<variable_data> Visiter::getVariables() {
    std::vector<variable_data> res;
    for (auto const &x : variables) {
        res.push_back(*x);
    }
    return res;
}
