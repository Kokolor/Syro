// ast.h

#ifndef AST_H
#define AST_H

#endif // AST_H

typedef enum
{
    AST_PLUS,
    AST_MINUS,
    AST_STAR,
    AST_SLASH,
    AST_NUMBER,
} NodeType;

typedef struct Node Node;

struct Node
{
    int operation;
    Node *left;
    Node *right;
    int number_value;
};

Node *make_node(int operation, Node *left, Node *right, int number_value);

Node *make_leaf(int operation, int number_value);

Node *make_unary(int operation, Node *left, int number_value);

int token_to_ast(Lexer *lexer, TokenType token);

Node *parse_binary_expression(Lexer *lexer);

int evaluate_ast(Lexer *lexer, Node *node);