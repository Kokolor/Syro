// ast.h

#ifndef AST_H
#define AST_H

#include "lexer/lexer.h"

typedef enum
{
    AST_PLUS,
    AST_MINUS,
    AST_STAR,
    AST_SLASH,
    AST_NUMBER,
    AST_PRINT,
    AST_EXPRESSION,
    AST_STATEMENT_LIST,
} NodeType;

typedef struct Node Node;

struct Node
{
    NodeType type;
    Node *left;
    Node *right;
    int number_value;
    Node *expression;
};

Node *make_node(NodeType type, Node *left, Node *right, int number_value);

Node *make_leaf(NodeType type, int number_value);

Node *make_print(Node *expression);

Node *make_statement_list(Node *list, Node *statement);

Node *parse_primary(Lexer *lexer);

Node *parse_binary_expression_with_precedence(Lexer *lexer, int precedence);

Node *parse_binary_expression(Lexer *lexer);

Node *parse_statement(Lexer *lexer);

Node *parse_statement_list(Lexer *lexer);

int token_to_ast(Lexer *lexer, TokenType token);

int get_operator_precedence(NodeType type);

int is_operator(TokenType token);

void free_ast(Node *node);

#endif // AST_H
