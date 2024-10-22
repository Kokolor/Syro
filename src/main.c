#include <stdio.h>
#include <stdlib.h>
#include "lexer/lexer.h"
#include "parser/ast.h"

int main()
{
    char source[] = "2 * 3 + 4 * 5";
    Lexer lexer;
    init_lexer(&lexer, source);

    Node *ast = parse_binary_expression(&lexer);

    int result = evaluate_ast(&lexer, ast);
    printf("Result: %d\n", result);

    free(ast);

    return 0;
}
