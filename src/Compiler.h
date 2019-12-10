#ifndef COMPILER_H
#define COMPILER_H

#include "Visiter.h"
#include "node.h"
#include <map>
#include <string>
#include <vector>

class Compiler {
private:
    std::vector<variable_data> variables;

    // TODO: floats
    void assignStack();
    std::string processNode(node_t *node, node_t *parent);
    std::string getMetaData(node_t *node, std::string path);
    std::string getFunctionName(node_t *function_node);
    std::string getFunctionBody(node_t *function_node);
    std::string assignmentOperator(node_t *node);
    std::string returnOperator(node_t *node);
    std::string getDeclarations();
    variable_data *findVariable(std::string name);

    std::string binOpAsm(node_t *left, node_t *right, token_t op);
    std::string unOpAsm(node_t *node, token_t op);

    std::string getMul(int offset);
    std::string getAdd(int offset);
    std::string getSub(int offset);
    std::string getEq(int offset);

    std::string dumpFloat(float num);

    std::string getOperator(int offset, token_t op);

    std::string arrayIndex(node_t *node);

public:
    Compiler();
    ~Compiler();
    static int tmp;

    void compileFunc(node_t *node, std::string output, std::string input);
    void setVariables(std::vector<variable_data> variables);
};

#endif /* ifndef COMPILER_H */
