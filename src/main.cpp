#include <iostream>
#include <fstream>

#include "token.h"
#include "Lexer.h"
#include "node.h"
#include "Parser.h"
#include "Visiter.h"
#include "Compiler.h"

int main(int argc, char **argv) {
    Lexer lexer;

    std::string text;
    if (argc < 3) {
        std::cout << "Provide valid paths to input and output files" << std::endl;
        return 1;
    } else {
        std::ifstream file(argv[1]);
        text.assign((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    }

    /* std::cout << text << std::endl; */
    auto lexemes = lexer.lex(text);

    /* for (auto i : lexemes) { */
    /*     std::cout << i.value << " " << i.type << std::endl; */
    /* } */

    Parser parser(lexemes);
    auto head = parser.parse();

    Visiter::visualize(head, 0);
    Visiter visiter;
    visiter.visitNodes(head, nullptr);
    /* visiter.printVariables(); */

    visiter.visualize(head->children.at(0)->children.at(0), 0);
    Compiler compiler;
    compiler.setVariables(visiter.getVariables());
    compiler.compileFunc(head->children.at(0)->children.at(0), argv[2], argv[1]);

    return 0;

}
