// ast.h

#ifndef AST_H
#define AST_H

#include <lexer/lexer.h>

typedef enum
{
    AST_PLUS,
    AST_MINUS,
    AST_STAR,
    AST_SLASH,
    AST_NUMBER,
    AST_IDENTIFIER,
    AST_VARIABLE_DECL,
    AST_FUNCTION_DECL,
    AST_FUNCTION_CALL,
    AST_RETURN_STMT,
    AST_PRINT,
    AST_STATEMENT_LIST,
    AST_CAST
} NodeType;

typedef struct Node Node;

struct Node
{
    NodeType type;
    Node *left;
    Node *right;
    int number_value;
    char *var_type;
    char *var_name;
    Node *expression;
    char *func_name;
    Node **parameters;
    int param_count;
    Node *body;
    char *return_type;
    char *cast_type;
};

Node *make_node(NodeType type, Node *left, Node *right, int number_value);
Node *make_function_decl(char *func_name, Node **parameters, int param_count, char *return_type, Node *body);
Node *make_variable_decl(char *var_type, char *var_name, Node *expression);
Node *make_return_stmt(Node *expression);
Node *make_leaf(NodeType type, int number_value);
Node *make_print(Node *expression);
Node *make_variable_ref(char *var_name);
Node *make_function_call(char *func_name, Node **arguments, int arg_count);
Node *make_statement_list(Node *list, Node *statement);
Node *make_cast(char *cast_type, Node *expression);

Node *parse_statement_list(Lexer *lexer);
Node *parse_binary_expression(Lexer *lexer);
NodeType token_to_ast(Lexer *lexer, TokenType token);
int get_operator_precedence(NodeType type);
int is_operator(TokenType token);
void free_ast(Node *node);

#endif // AST_H
