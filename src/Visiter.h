#ifndef VISITER_H
#define VISITER_H

#include <map>
#include <string>
#include <memory>

#include "node.h"

struct variable_data {
    variable_data(std::string name, int size, std::string type, std::string value, int stack_offset, int array_size=0);
    std::string name;
    int size;
    std::string type;
    std::string value;
    int stack_offset;
    int array_size;
};

class Visiter {
private:
    std::vector<std::unique_ptr<variable_data>> variables{};
    std::map<std::string, std::string> functions{};

    void variableDeclaration(node_t *node, node_t *parent);
    void functionDeclaration(node_t *node, node_t *parent);
    void functionCall(node_t *node, node_t *parent);
    void variable(node_t *node, node_t *parent);
    void binaryExpression(node_t *node, node_t *parent);
    void unaryExpression(node_t *node, node_t *parent);
    void assignment(node_t *node, node_t *parent);
    void arrayIndex(node_t *node, node_t *parent);

    bool variablePresent(std::string name);
    bool isInteger(std::string value);
    bool isPositiveInteger(std::string value);
    std::string typeOf(node_t *node);
    bool areCompatible(std::string type1, std::string type2);
    bool isPointer(std::string type);
    std::string biggerType(std::string type1, std::string type2);
    std::string nodeToString(node_t *node);
    std::string stripPointer(std::string type, int depth);

public:
    Visiter();
    virtual ~Visiter();

    static void visualize(node_t *head, int depth);

    void visitNodes(node_t *node, node_t *parent);
    void printVariables();
    std::vector<variable_data> getVariables();
};

#endif /* VISITER_H */
