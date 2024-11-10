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

    AST_EQUAL_EQUAL,
    AST_BANG_EQUAL,
    AST_LESS,
    AST_LESS_EQUAL,
    AST_GREATER,
    AST_GREATER_EQUAL,

    AST_ASSIGNMENT,
    AST_NUMBER,
    AST_IDENTIFIER,
    AST_VARIABLE_DECL,
    AST_FUNCTION_DECL,
    AST_FUNCTION_CALL,
    AST_RETURN_STMT,
    AST_PRINT,
    AST_STATEMENT_LIST,
    AST_CAST,
    AST_IF_STATEMENT,
    AST_WHILE_STATEMENT,
    AST_FOR_STATEMENT,

    AST_ADDRESS_OF,
    AST_DEREFERENCE,
    AST_DEREFERENCE_ASSIGNMENT,
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
    Node *condition;
    Node *then_branch;
    Node *else_branch;
    Node *init;
    Node *increment;
};

Node *make_node(NodeType type, Node *left, Node *right, int number_value);
Node *make_leaf(NodeType type, int number_value);
Node *make_assignment(char *var_name, Node *expression);
Node *make_dereference_assignment(Node *dereferenced_expr, Node *value_expr);
Node *make_function_decl(char *func_name, Node **parameters, int param_count, char *return_type, Node *body);
Node *make_variable_decl(char *var_type, char *var_name, Node *expression);
Node *make_return_stmt(Node *expression);
Node *make_print(Node *expression);
Node *make_variable_ref(char *var_name);
Node *make_function_call(char *func_name, Node **arguments, int arg_count);
Node *make_statement_list(Node *list, Node *statement);
Node *make_cast(char *cast_type, Node *expression);
Node *make_if_statement(Node *condition, Node *then_branch, Node *else_branch);
Node *make_while_statement(Node *condition, Node *body);
Node *make_for_statement(Node *init, Node *condition, Node *increment, Node *body);
Node *make_address_of(Node *expression);
Node *make_dereference(Node *expression);

char *parse_type(Lexer *lexer);
Node *parse_if_statement(Lexer *lexer);
Node *parse_while_statement(Lexer *lexer);
Node *parse_for_statement(Lexer *lexer);
Node *parse_statement(Lexer *lexer);
Node *parse_statement_list(Lexer *lexer);
Node *parse_binary_expression(Lexer *lexer);
Node *parse_primary(Lexer *lexer);
Node *parse_expression_statement(Lexer *lexer);
Node *parse_unary_expression(Lexer *lexer);
Node *parse_binary_expression_with_precedence(Lexer *lexer, int precedence);
NodeType token_to_ast(Lexer *lexer, TokenType token);
int is_type_token(TokenType token);
int get_operator_precedence(NodeType type);
int is_operator(TokenType token);
void free_ast(Node *node);

#endif // AST_H
