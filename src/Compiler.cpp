#include "Compiler.h"
#include "Visiter.h"
#include "node.h"
#include "token.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

Compiler::Compiler() = default;
Compiler::~Compiler() = default;

int Compiler::tmp = 0;

void Compiler::compileFunc(node_t *node, std::string output, std::string input) {
    std::ofstream outfile(output);
    assignStack();
    outfile << getMetaData(node, input) << std::endl;
    outfile << getFunctionName(node) << std::endl;
    outfile << getFunctionBody(node) << std::endl;
    outfile << "\t.ident \"Arjaz-C-Compiler\"" << std::endl;
    outfile << "\t.section .note.GNU-stack,\"\",@progbits" << std::endl;
    outfile.close();
}

std::string Compiler::getMetaData(node_t *node, std::string path) {
    return "\t.file\t\"" + path + "\"\n\t.text\n\t.globl\t" +
        node->children.at(1)->token.value + "\n\t.type\t" +
        node->children.at(1)->token.value + ", @function";
}

std::string Compiler::getFunctionName(node_t *function_node) {
    return function_node->children.at(1)->token.value + ":";
}

std::string Compiler::processNode(node_t *node, node_t *parent) {
    std::string result = "";
    if (node->type == UN_OP_NODE) {
        result = unOpAsm(node->children.at(0), node->token);
        auto new_var = variables.back();
        variables.pop_back();
        auto var = *findVariable(node->children.at(0)->token.value);
        result += "\tmovl -" + std::to_string(new_var.stack_offset) + "(%rbp), %eax\n";
        result += "\tmovl %eax, -" + std::to_string(var.stack_offset) + "(%rbp)\n";
    } else if (node->type == ASSIGNMENT_NODE) {
        result = assignmentOperator(node);
    } else if (node->type == RETURN_NODE) {
        result = returnOperator(node);
    }
    return result;
}

std::string Compiler::returnOperator(node_t *node) {
    auto node_type = node->children.at(0)->type;
    std::string res = "\n//\treturn\n";
    if (node_type == VAR_NODE) {
        auto var = findVariable(node->children.at(0)->token.value);
        res += "\tmovl -" + std::to_string(var->stack_offset) + "(%rbp), %eax\n";
        return res;
    } else if (node_type == NUM_NODE) {
        res += "\tmovl $" + node->children.at(0)->token.value + ", %eax\n";
        return res;
    } else if (node_type == BIN_OP_NODE) {
        // TODO: add left, add right
    }
    return "";
}

// Let's pretend that the starting values are ints
std::string Compiler::getDeclarations() {
    std::string result = "";
    for (auto &x : variables) {
        result += "// " + x.name + " = " + x.value + "\n";
        result += "\tmovl $" + x.value + ", -" + std::to_string(x.stack_offset) + "(%rbp)\n";
    }
    return result;
}

std::string Compiler::getFunctionBody(node_t *function_node) {
    // std::cout << "getFunctionBody()" << std::endl;
    std::string debug = "";
    for (const auto &x : variables) {
        debug += "//\t" + x.name + ": -" + std::to_string(x.stack_offset) + "(%rbp)\n";
    }
    std::string init = ".LFB0:\n\tpushq %rbp\n\tmovq %rsp, %rbp\n\n";
    std::string declarations = getDeclarations();
    std::string exit = "\n\tpopq %rbp\n\tret\n.LFE0:\n\t.size " + function_node->children.at(1)->token.value + ", .-" + function_node->children.at(1)->token.value;
    std::string body = "";

    for (auto child : function_node->children.at(3)->children) {
        body += processNode(child, function_node->children.at(3));
    }

    return debug + init + declarations + body + exit;
}

void Compiler::assignStack() {
    const int stack_size = 4;
    int stack = stack_size;
    for (auto &x : variables) {
        if (x.array_size) {
            x.stack_offset = stack;
            stack += stack_size * x.array_size;
        } else {
            x.stack_offset = stack;
            stack += stack_size;
        }
    }
}

std::string Compiler::assignmentOperator(node_t *node) {
    /* std::cout << "assignmentOperator()" << std::endl; */
    std::string result = "";
    auto var = *findVariable(node->children.at(0)->token.value);
    auto right = node->children.at(1);
    if (right->type == NUM_NODE) {
        result += "\tmovl $" + right->token.value + ", -" + std::to_string(var.stack_offset) + "(%rbp)\n";
    } else if (right->type == VAR_NODE) {
        auto left_var = *findVariable(right->token.value);
        result += "\tmovl -" + std::to_string(left_var.stack_offset) + "(%rbp), %eax\n";
        result += "\tmovl %eax, -" + std::to_string(var.stack_offset) + "(%rbp)\n";
    } else if (right->type == UN_OP_NODE) {
        result += unOpAsm(right, node->token);
    } else if (right->type == BIN_OP_NODE) {
        result += binOpAsm(right->children.at(0), right->children.at(1), right->token);
        result += "\tmovl -" + std::to_string(variables.back().stack_offset) + "(%rbp), %eax\n";
        result += "\tmovl %eax, -" + std::to_string(var.stack_offset) + "(%rbp)\n";
        variables.pop_back();
    } else if (right->type == ARRAY_INDEX_NODE) {
        result += arrayIndex(right);
        result += "\tmovl -" + std::to_string(variables.back().stack_offset) + "(%rbp), %eax\n";
        result += "\tmovl %eax, -" + std::to_string(var.stack_offset) + "(%rbp)\n";
        variables.pop_back();
    }
    return result;
}

void Compiler::setVariables(std::vector<variable_data> variables) {
    this->variables = variables;
}

// Returns a pointer to the variable named `name` if it's present, otherwise `nullptr`
variable_data *Compiler::findVariable(std::string name) {
    for (auto &x : variables) {
        if (x.name == name) {
            return &x;
        }
    }
    return nullptr;
}

// Multuplies a value stored in the `%eax` by the `-offset(%rbp)` and then writes the result to the `-offset(%rbp)`
std::string Compiler::getMul(int offset) {
    std::string res;
    res = "\tmull -" + std::to_string(offset) + "(%rbp)\n";
    res += "\tmovl %eax, -" + std::to_string(offset) + "(%rbp)\n";
    return res;
}

// Adds a value stored in the `%eax` to the `-offset(%rbp)`
std::string Compiler::getAdd(int offset) {
    return "\taddl %eax, -" + std::to_string(offset) + "(%rbp)\n";
}

// Subs a value stored in the `%eax` from the `-offset(%rbp)`
std::string Compiler::getSub(int offset) {
    return "\tsubl %eax, -" + std::to_string(offset) + "(%rbp)\n";
}

// Eqs a value stored in the `%eax` from the `-offset(%rbp)`
std::string Compiler::getEq(int offset) {
    return "\tandl %eaxl -" + std::to_string(offset) + "(%rbp)\n";
}

// Performs unary operator `op` on `node` and pushes the result to the stack
// `node` is variable
std::string Compiler::unOpAsm(node_t *node, token_t op) {
    std::string res = "";

    int offset = variables.back().stack_offset + 4;
    variables.push_back(variable_data("tmp" + std::to_string(Compiler::tmp++), 4, "int", "0", offset));
    auto new_var = variables.back();
    auto var = *findVariable(node->token.value);

    if (op.type == DEC || op.type == INC) {
        if (node->type == NUM_NODE) {
            throw std::logic_error(NUM_NODE + " doesn't support " + op.type + " operator");
        } else if (node->type == VAR_NODE) {
            res = "\tmovl -" + std::to_string(var.stack_offset) + "(%rbp), %eax\n";
            if (op.type == DEC) {
                res += "\tdec %eax\n";
            } else {
                res += "\tinc %eax\n";
            }
            res += "\tmovl %eax, -" + std::to_string(offset) + "(%rbp)\n";
        } else {
            throw std::logic_error(op.type + " is not implemented.");
        }
    } else {
        throw std::logic_error(op.type + " is not implemented.");
    }

    return res;
}

// Performs binary operator `op` on `left` and `right` and pushes the result to the stack
std::string Compiler::binOpAsm(node_t *left, node_t *right, token_t op) {
    // std::cout << "binOpAsm()" << std::endl;
    std::string result = "";

    int offset = variables.back().stack_offset + 4;
    variables.push_back(variable_data("tmp" + std::to_string(Compiler::tmp++), 4, "int", "0", offset));
    auto bin_var = variables.back();

    if ((left->type == VAR_NODE || left->type == NUM_NODE) && (right->type == VAR_NODE || right->type == NUM_NODE)) {
        // std::cout << "two nums/vars" << std::endl;
        std::string left_val;
        std::string right_val;
        if (left->type == NUM_NODE) {
            left_val = "$" + left->token.value;
        } else {
            auto var = findVariable(left->token.value);
            left_val = "-" + std::to_string(var->stack_offset) + "(%rbp)\n";
        }
        if (right->type == NUM_NODE) {
            right_val = "$" + right->token.value;
        } else {
            auto var = findVariable(right->token.value);
            right_val = "-" + std::to_string(var->stack_offset) + "(%rbp)\n";
        }

        // load left value to eax
        result += "\tmovl " + left_val + ", %eax\n";
        // load left value on stack
        result += "\tmovl %eax, -" + std::to_string(offset) + "(%rbp)\n";
        // load right value to eax
        result += "\tmovl " + right_val + ", %eax\n";
        // perform operator and store in the stack
        result += getOperator(offset, op);
    } else if ((left->type == VAR_NODE || left->type == NUM_NODE) && (right->type == BIN_OP_NODE || right->type == ARRAY_INDEX_NODE)) {
        // std::cout << "a var/num and a bin_op" << std::endl;
        std::string left_val;
        if (left->type == NUM_NODE) {
            left_val = "$" + left->token.value;
        } else {
            auto var = findVariable(left->token.value);
            left_val = "-" + std::to_string(var->stack_offset) + "(%rbp)\n";
        }

        if (right->type == ARRAY_INDEX_NODE) {
            result += arrayIndex(right);
        } else {
            result += binOpAsm(right->children.at(0), right->children.at(1), right->token);
        }
        auto right_var = variables.back();
        variables.pop_back();

        // load left value to eax
        result += "\tmovl " + left_val + ", %eax\n";
        // load left value on stack
        result += "\tmovl %eax, -" + std::to_string(offset) + "(%rbp)\n";
        // load right value to eax
        result += "\tmovl -" + std::to_string(right_var.stack_offset) + "(%rbp), %eax\n";
        // perform operator and store in the stack
        result += getOperator(offset, op);
    } else if ((right->type == VAR_NODE || right->type == NUM_NODE) && (left->type == BIN_OP_NODE || left->type == ARRAY_INDEX_NODE)) {
        // std::cout << "a bin_op and a var/num" << std::endl;
        std::string right_val;
        if (right->type == NUM_NODE) {
            right_val = "$" + right->token.value;
        } else {
            auto var = findVariable(right->token.value);
            right_val = "-" + std::to_string(var->stack_offset) + "(%rbp)\n";
        }
        if (right->type == ARRAY_INDEX_NODE) {
            result += arrayIndex(left);
        } else {
            result += binOpAsm(left->children.at(0), left->children.at(1), left->token);
        }
        auto left_var = variables.back();
        variables.pop_back();

        // load left value to eax
        result += "\tmovl -" + std::to_string(left_var.stack_offset) + "(%rbp), %eax\n";
        // load left value on stack
        result += "\tmovl %eax, -" + std::to_string(offset) + "(%rbp)\n";
        // load right value to eax
        result += "\tmovl " + right_val + ", %eax\n";
        // perform operator and store in the stack
        result += getOperator(offset, op);
    } else if ((right->type == BIN_OP_NODE || right->type == ARRAY_INDEX_NODE) && (left->type == BIN_OP_NODE || left->type == ARRAY_INDEX_NODE)) {
        // std::cout << "two bin_ops" << std::endl;
        if (right->type == ARRAY_INDEX_NODE) {
            result += arrayIndex(left);
        } else {
            result += binOpAsm(left->children.at(0), left->children.at(1), left->token);
        }
        auto left_var = variables.back();
        if (right->type == ARRAY_INDEX_NODE) {
            result += arrayIndex(right);
        } else {
            result += binOpAsm(right->children.at(0), right->children.at(1), right->token);
        }
        auto right_var = variables.back();
        variables.pop_back();
        variables.pop_back();

        // load left value to eax
        result += "\tmovl -" + std::to_string(left_var.stack_offset) + "(%rbp), %eax\n";
        // load left value on stack
        result += "\tmovl %eax, -" + std::to_string(offset) + "(%rbp)\n";
        // load right value on stack
        result += "\tmovl -" + std::to_string(right_var.stack_offset) + "(%rbp), %eax\n";
        result += getOperator(offset, op);
    }
    return result;
}

// Pushes a variable to the stack
std::string Compiler::arrayIndex(node_t *node) {
    std::string res = "";
    auto var = *findVariable(node->children.at(0)->token.value);
    int offset = variables.back().stack_offset + 4;
    variables.push_back(variable_data("tmp" + std::to_string(Compiler::tmp++), 4, "int", "0", offset));
    auto new_var = variables.back();

    if (node->children.at(1)->type == NUM_NODE) {
        res += "\tmovl -" + std::to_string(var.stack_offset + std::stoi(node->children.at(1)->token.value) * 4) + "(%rbp), %eax\n";
        res += "\tmovl %eax, -" + std::to_string(new_var.stack_offset) + "(%rbp)\n";
    } else if (node->children.at(1)->type == VAR_NODE) {
        // some shit with lea
    }

    return res;
}

// Performs operator `op` between `%eax` and `-offset(%rbp)` and places result to the `-offset(%rbp)`
std::string Compiler::getOperator(int offset, token_t op) {
    std::string res;
    if (op.type == PLUS) {
        res = getAdd(offset);
    } else if (op.type == MINUS) {
        res = getSub(offset);
    } else if (op.type == ASTERISK) {
        res = getMul(offset);
    } else if (op.type == AND) {
        res = getMul(offset);
    } else if (op.type == EQ) {
        res = getEq(offset);
    }
    return res;
}
