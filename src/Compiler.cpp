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
int Compiler::mark = 1;
int Compiler::float_offset = 0;

void Compiler::compileFunc(node_t *node, std::string output, std::string input) {
    std::ofstream outfile(output);
    assignStack();
    std::cout << node->type << std::endl;
    outfile << getMetaData(node, input) << std::endl;
    outfile << getFunctionName(node) << std::endl;
    outfile << getFunctionBody(node) << std::endl;
    outfile << "\t.ident \"Arjaz-C-Compiler\"" << std::endl;
    outfile << "\t.section .note.GNU-stack,\"\",@progbits" << std::endl;
    outfile.close();
}

std::string Compiler::getMetaData(node_t *node, std::string path) {
    std::cout << "getMetaData()" << std::endl;
    // return "\t.file\t\"" + path + "\"\n\t.section .text\n\t.globl\t" +
    return "\t.file\t\"" + path + "\"\n\t.section .data\n\tdata_section:\n\t.section .text\n\t.globl\t" +
        node->children.at(1)->token.value + "\n\t.type\t" +
        node->children.at(1)->token.value + ", @function\n";
}

std::string Compiler::getFunctionName(node_t *function_node) {
    std::cout << "getFunctionName()" << std::endl;
    return function_node->children.at(1)->token.value + ":";
}

std::string Compiler::processNode(node_t *node, node_t *parent) {
    std::cout << "processNode()" << std::endl;
    std::string result = "";
    if (node->type == UN_OP_NODE) {
        result = unOpAsm(node->children.at(0), node->token);
        auto new_var = variables.back();
        variables.pop_back();
        auto var = *findVariable(node->children.at(0)->token.value);
        result += "\tmovl -" + std::to_string(new_var.stack_offset) + "(%ebp), %eax\n";
        result += "\tmovl %eax, -" + std::to_string(var.stack_offset) + "(%ebp)\n";
    } else if (node->type == ASSIGNMENT_NODE) {
        result = assignmentOperator(node);
    } else if (node->type == RETURN_NODE) {
        result = returnOperator(node);
    }
    return result;
}

std::string Compiler::returnOperator(node_t *node) {
    std::cout << "returnOperator()" << std::endl;
    auto node_type = node->children.at(0)->type;
    std::string result = "\n//\treturn\n";
    if (node_type == VAR_NODE) {
        auto var = findVariable(node->children.at(0)->token.value);
        if (var->type == "float" || var->type == "double") {
            result += "\tflds -" + std::to_string(var->stack_offset) + "(%ebp)\n";
            // result += "\tmovss -" + std::to_string(var->stack_offset) + "(%ebp), %xmm0\n";
        } else {
            result += "\tmovl -" + std::to_string(var->stack_offset) + "(%ebp), %eax\n";
        }
    } else if (node_type == NUM_NODE) {
        result += "\tmovl $" + node->children.at(0)->token.value + ", %eax\n";
    } else if (node_type == BIN_OP_NODE) {
        // TODO: add left, add right
    }
    return result;
}

// Let's pretend that the starting values are ints
std::string Compiler::getDeclarations() {
    std::cout << "getDeclarations()" << std::endl;
    std::string result = "";
    result += "\tsubl $" + std::to_string(variables.back().stack_offset) + ", %esp\n";
    for (auto &x : variables) {
        result += "// " + x.name + " = " + x.value + "\n";
        result += "\tmovl $" + x.value + ", -" + std::to_string(x.stack_offset) + "(%ebp)\n";
    }
    return result;
}

std::string Compiler::getFunctionBody(node_t *function_node) {
    std::cout << "getFunctionBody()" << std::endl;
    std::string debug = "";
    for (const auto &x : variables) {
        debug += "//\t" + x.name + ": -" + std::to_string(x.stack_offset) + "(%ebp)\n";
    }
    std::string init = ".LFB0:\n\tpush %ebp\n\tmovl %esp, %ebp\n\n";
    std::string declarations = getDeclarations();
    std::string exit = "\n\tmovl %ebp, %esp\n";
    // std::string exit = "\n\tmovl %esp, %ebp\n";
    exit += "\tpop %ebp\n";
    exit += "\tret\n";
    exit += ".LFE0:\n";
    exit += "\t.size " + function_node->children.at(1)->token.value + ", .-" + function_node->children.at(1)->token.value;
    std::string body = "";

    for (auto child : function_node->children.at(3)->children) {
        body += processNode(child, function_node->children.at(3));
    }

    return debug + init + declarations + body + exit;
}

void Compiler::assignStack() {
    std::cout << "assignStack()" << std::endl;
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
    std::cout << "assignmentOperator()" << std::endl;
    std::string result = "";
    int offset;
    if (node->children.at(0)->type == VAR_NODE) {
        offset = findVariable(node->children.at(0)->token.value)->stack_offset;
    } else if (node->children.at(0)->type == ARRAY_INDEX_NODE) {
        auto var = findVariable(node->children.at(0)->children.at(0)->token.value);
        if (node->children.at(0)->children.at(1)->type == NUM_NODE) {
            offset = var->stack_offset + 4 * std::stoi(node->children.at(0)->children.at(1)->token.value);
        } else if (node->children.at(0)->children.at(1)->type == VAR_NODE) {
            result += "//\tnot implemented";
        }
    } else {
        throw std::logic_error("You can assign only to vars and array items");
    }
    auto right = node->children.at(1);
    if (right->type == NUM_NODE) {
        if (isInt(right->token.value)) {
            result += "\tmovl $" + right->token.value + ", -" + std::to_string(offset) + "(%ebp)\n";
        } else {
            // that's float
            result += dumpFloat(right->token.value);
            result += "\tmovl (" + std::to_string(Compiler::float_offset) + " + data_section), %eax\n";
            Compiler::float_offset += 4;
            result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
        }
    } else if (right->type == VAR_NODE) {
        auto left_var = *findVariable(right->token.value);
        result += "\tmovl -" + std::to_string(left_var.stack_offset) + "(%ebp), %eax\n";
        result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
    } else if (right->type == UN_OP_NODE) {
        result += unOpAsm(right, node->token);
    } else if (right->type == BIN_OP_NODE) {
        result += binOpAsm(right->children.at(0), right->children.at(1), right->token);
        auto var = findVariable(node->children.at(0)->token.value);
        Visiter visiter;
        visiter.setVariables(variables);
        if ((var->type == "float" || var->type == "double") && visiter.typeOf(right) != "float" && visiter.typeOf(right) != "double") {
            result += "\tfild -" + std::to_string(variables.back().stack_offset) + "(%ebp)\n";
            result += "\tfstp -" + std::to_string(offset) + "(%ebp)\n";
        } else {
            result += "\tmovl -" + std::to_string(variables.back().stack_offset) + "(%ebp), %eax\n";
            result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
        }
        variables.pop_back();
    } else if (right->type == ARRAY_INDEX_NODE) {
        result += arrayIndex(right);
        result += "\tmovl -" + std::to_string(variables.back().stack_offset) + "(%ebp), %eax\n";
        result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
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

// Multuplies a value stored in the `%eax` by the `-offset(%ebp)` and then writes the result to the `-offset(%ebp)`
std::string Compiler::getMul(int offset) {
    std::string result;
    result = "\tmull -" + std::to_string(offset) + "(%ebp)\n";
    result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
    return result;
}

// Adds a value stored in the `%eax` to the `-offset(%ebp)`
std::string Compiler::getAdd(int offset) {
    return "\taddl %eax, -" + std::to_string(offset) + "(%ebp)\n";
}

// Subs a value stored in the `%eax` from the `-offset(%ebp)`
std::string Compiler::getSub(int offset) {
    return "\tsubl %eax, -" + std::to_string(offset) + "(%ebp)\n";
}

// Compares a value stored in the `%eax` and the `-offset(%ebp)`
std::string Compiler::getEq(int offset) {
    std::string result = "";
    result += "\tcmpl %eax, -" + std::to_string(offset) + "(%ebp)\n";
    result += "\tjnz " + std::to_string(Compiler::mark) + "f\n";
    result += "\tmovl $1, -" + std::to_string(offset) + "(%ebp)\n";
    result += "\tjmp " + std::to_string(Compiler::mark + 1) + "f\n";
    result += std::to_string(Compiler::mark++) + ":\n";
    result += "\tmovl $0, -" + std::to_string(offset) + "(%ebp)\n";
    result += std::to_string(Compiler::mark++) + ":\n";
    return result;
}

// Compares a float value stored in the `-offset1(%ebp)` and the `-offset2(%ebp)` and pushes the result to the second location. offset2 must contain float
std::string Compiler::getFloatEq(int offset1, int offset2, bool isLeftFloat) {
    std::string result = "";
    if (isLeftFloat) {
        std::cout << "Left is float" << std::endl;
        // result += "\tflds -" + std::to_string(offset1) + "(%ebp)\n";
        // result += "\tflds -" + std::to_string(offset2) + "(%ebp)\n";
        // result += "\tfcomip %st(1), %st(0)\n";
        // result += "\tfstp %st(0)\n";
        result += "\tmovl -" + std::to_string(offset1) + "(%ebp), %eax\n";
        result += "\tcmpl %eax, -" + std::to_string(offset2) + "(%ebp)\n";
        result += "\tjne " + std::to_string(Compiler::mark) + "f\n";
        result += "\tmovl $1, -" + std::to_string(offset2) + "(%ebp)\n";
        result += "\tjmp " + std::to_string(Compiler::mark + 1) + "f\n";
        result += std::to_string(Compiler::mark++) + ":\n";
        result += "\tmovl $0, -" + std::to_string(offset2) + "(%ebp)\n";
        result += std::to_string(Compiler::mark++) + ":\n";
    } else {
        result += "\tfilds -" + std::to_string(offset1) + "(%ebp)\n";
        result += "\tflds -" + std::to_string(offset2) + "(%ebp)\n";
        result += "\tfcomip %st(1), %st(0)\n";
        result += "\tfstp %st(0)\n";
        result += "\tjne " + std::to_string(Compiler::mark) + "f\n";
        result += "\tmovl $1, -" + std::to_string(offset2) + "(%ebp)\n";
        result += "\tjmp " + std::to_string(Compiler::mark + 1) + "f\n";
        result += std::to_string(Compiler::mark++) + ":\n";
        result += "\tmovl $0, -" + std::to_string(offset2) + "(%ebp)\n";
        result += std::to_string(Compiler::mark++) + ":\n";
    }
    return result;
}

// Performs unary operator `op` on `node` and pushes the result to the stack
// `node` is variable
std::string Compiler::unOpAsm(node_t *node, token_t op) {
    std::string result = "";

    result += "\tsubl $4, %esp\n";

    int offset = variables.back().stack_offset + 4;
    variables.push_back(variable_data("tmp" + std::to_string(Compiler::tmp++), 4, "int", "0", offset));
    // auto new_var = variables.back();
    auto var = *findVariable(node->token.value);

    if (op.type == DEC || op.type == INC) {
        if (node->type == NUM_NODE) {
            throw std::logic_error(NUM_NODE + " doesn't support " + op.type + " operator");
        } else if (node->type == VAR_NODE) {
            result += "\tmovl -" + std::to_string(var.stack_offset) + "(%ebp), %eax\n";
            if (op.type == DEC) {
                result += "\tdec %eax\n";
            } else {
                result += "\tinc %eax\n";
            }
            result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
        } else {
            throw std::logic_error(op.type + " is not implemented.");
        }
    } else {
        throw std::logic_error(op.type + " is not implemented.");
    }

    return result;
}

// Performs binary operator `op` on `left` and `right` and pushes the result to the stack
std::string Compiler::binOpAsm(node_t *left, node_t *right, token_t op) {
    std::cout << "binOpAsm()" << std::endl;
    std::string result = "";
    result += "\tsubl $4, %esp\n";

    int offset = variables.back().stack_offset + 4;
    variables.push_back(variable_data("tmp" + std::to_string(Compiler::tmp++), 4, "int", "0", offset));
    auto bin_var = variables.back();

    if ((left->type == VAR_NODE || left->type == NUM_NODE) && (right->type == VAR_NODE || right->type == NUM_NODE)) {
        // std::cout << "two nums/vars" << std::endl;
        std::string left_val;
        std::string right_val;
        bool isLeftFloat = false;
        bool isRightFloat = false;
        if (left->type == NUM_NODE) {
            left_val = "$" + left->token.value;
        } else {
            auto var = findVariable(left->token.value);
            if (var->type == "double" || var->type == "float") {
                isLeftFloat = true;
            }
            left_val = "-" + std::to_string(var->stack_offset) + "(%ebp)";
            result += "// left variable is float/double\n";
        }
        if (right->type == NUM_NODE) {
            right_val = "$" + right->token.value;
        } else {
            auto var = findVariable(right->token.value);
            if (var->type == "double" || var->type == "float") {
                isRightFloat = true;
            }
            right_val = "-" + std::to_string(var->stack_offset) + "(%ebp)";
            result += "// right variable is float/double\n";
        }

        // load left value to eax
        result += "\tmovl " + left_val + ", %eax\n";
        // load left value on stack
        result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
        // load right value to eax
        result += "\tmovl " + right_val + ", %eax\n";
        // check float flags
        if (isLeftFloat && isRightFloat) {
            result += "\tsubl $4, %esp\n";
            int new_offset = this->variables.back().stack_offset + 4;
            result += "\tmovl %eax, -" + std::to_string(new_offset) + "(%ebp)\n";
            if (op.type == EQ) {
                result += getFloatEq(new_offset, offset, true);
            } else {
                throw std::logic_error("Operator is not implemented for floating points");
            }
        } else if (isLeftFloat) {
            result += "\tsubl $4, %esp\n";
            int new_offset = this->variables.back().stack_offset + 4;
            result += "\tmovl " + left_val + ", %eax\n";
            result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
            result += "\tmovl " + right_val + ", %eax\n";
            result += "\tmovl %eax, -" + std::to_string(new_offset) + "(%ebp)\n";
            if (op.type == EQ) {
                result += getFloatEq(new_offset, offset, false);
            } else {
                throw std::logic_error("Operator is not implemented for floating points");
            }
        } else if (isRightFloat) {
            result += "\tsubl $4, %esp\n";
            int new_offset = this->variables.back().stack_offset + 4;
            result += "\tmovl " + right_val + ", %eax\n";
            result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
            result += "\tmovl " + left_val + ", %eax\n";
            result += "\tmovl %eax, -" + std::to_string(new_offset) + "(%ebp)\n";
            if (op.type == EQ) {
                result += getFloatEq(new_offset, offset, false);
            } else {
                throw std::logic_error("Operator is not implemented for floating points");
            }
        } else {
            // performs operator and store in the stack
            result += getOperator(offset, op);
        }
    } else if ((left->type == VAR_NODE || left->type == NUM_NODE) && (right->type == BIN_OP_NODE || right->type == ARRAY_INDEX_NODE)) {
        // std::cout << "a var/num and a bin_op" << std::endl;
        std::string left_val;
        if (left->type == NUM_NODE) {
            left_val = "$" + left->token.value;
        } else {
            auto var = findVariable(left->token.value);
            left_val = "-" + std::to_string(var->stack_offset) + "(%ebp)";
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
        result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
        // load right value to eax
        result += "\tmovl -" + std::to_string(right_var.stack_offset) + "(%ebp), %eax\n";
        // perform operator and store in the stack
        result += getOperator(offset, op);
    } else if ((right->type == VAR_NODE || right->type == NUM_NODE) && (left->type == BIN_OP_NODE || left->type == ARRAY_INDEX_NODE)) {
        // std::cout << "a bin_op and a var/num" << std::endl;
        std::string right_val;
        if (right->type == NUM_NODE) {
            right_val = "$" + right->token.value;
        } else {
            auto var = findVariable(right->token.value);
            right_val = "-" + std::to_string(var->stack_offset) + "(%ebp)";
        }
        if (right->type == ARRAY_INDEX_NODE) {
            result += arrayIndex(left);
        } else {
            result += binOpAsm(left->children.at(0), left->children.at(1), left->token);
        }
        auto left_var = variables.back();
        variables.pop_back();

        // load left value to eax
        result += "\tmovl -" + std::to_string(left_var.stack_offset) + "(%ebp), %eax\n";
        // load left value on stack
        result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
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
        result += "\tmovl -" + std::to_string(left_var.stack_offset) + "(%ebp), %eax\n";
        // load left value on stack
        result += "\tmovl %eax, -" + std::to_string(offset) + "(%ebp)\n";
        // load right value on stack
        result += "\tmovl -" + std::to_string(right_var.stack_offset) + "(%ebp), %eax\n";
        result += getOperator(offset, op);
    }
    return result;
}

// Pushes a variable to the stack
std::string Compiler::arrayIndex(node_t *node) {
    std::string result = "";

    result += "\tsubl $4, %esp\n";

    auto var = *findVariable(node->children.at(0)->token.value);
    int offset = variables.back().stack_offset + 4;
    variables.push_back(variable_data("tmp" + std::to_string(Compiler::tmp++), 4, "int", "0", offset));
    auto new_var = variables.back();

    if (node->children.at(1)->type == NUM_NODE) {
        result += "\tmovl -" + std::to_string(var.stack_offset + std::stoi(node->children.at(1)->token.value) * 4) + "(%ebp), %eax\n";
        result += "\tmovl %eax, -" + std::to_string(new_var.stack_offset) + "(%ebp)\n";
    } else if (node->children.at(1)->type == VAR_NODE) {
        auto index = findVariable(node->children.at(1)->token.value);
        result += "// array is " + var.name + "\n";
        result += "\tleal -" + std::to_string(var.stack_offset) + "(%ebp), %esi\n";
        result += "// index is " + index->name + "\n";
        result += "\tmovl -" + std::to_string(index->stack_offset) + "(%ebp), %eax\n";
        result += "\tneg %eax\n";
        result += "\tmovl (%esi, %eax, 4), %eax\n";
        result += "\tmovl %eax, -" + std::to_string(new_var.stack_offset) + "(%ebp)\n";
    }

    return result;
}

std::string Compiler::getFloatOperator(int offset1, int offset2, token_t op) {
    std::string result;
    if (op.type == EQ) {
        // result = getFloatEq(offset1, offset2);
    }
    return result;
}

// Performs operator `op` between `%eax` and `-offset(%ebp)` and places the result to the `-offset(%ebp)`
std::string Compiler::getOperator(int offset, token_t op) {
    std::string result;
    if (op.type == PLUS) {
        result = getAdd(offset);
    } else if (op.type == MINUS) {
        result = getSub(offset);
    } else if (op.type == ASTERISK) {
        result = getMul(offset);
    } else if (op.type == AND) {
        result = getMul(offset);
    } else if (op.type == EQ) {
        result = getEq(offset);
    }
    return result;
}

// Dumps .float `num` with section handling
std::string Compiler::dumpFloat(std::string num) {
    std::string result = "";
    result += "\t.section .data\n";
    result += "\t.float " + num + "\n";
    result += "\t.section .text\n";
    return result;
}

// Return `true` if `value` can be converted to int
bool Compiler::isInt(std::string value) {
    if (value.empty() || ((!isdigit(value[0]) && (value[0] != '-') && (value[0] != '+'))))
        return false;
    char *p;
    strtol(value.c_str(), &p, 10);

    return (*p == 0);
}
