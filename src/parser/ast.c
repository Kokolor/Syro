// ast.c

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

Node *make_node(int operation, Node *left, Node *right, int number_value)
{
    Node *node;

    node = (Node *)malloc(sizeof(Node));
    if (node == NULL)
    {
        fprintf(stderr, "[ast.c][make_node]: Memory allocation failed for a new AST node.\n");
        exit(EXIT_FAILURE);
    }

    node->operation = operation;
    node->left = left;
    node->right = right;
    node->number_value = number_value;

    return node;
}

Node *make_leaf(int operation, int number_value)
{
    return make_node(operation, NULL, NULL, number_value);
}

Node *make_unary(int operation, Node *left, int number_value)
{
    return make_node(operation, left, NULL, number_value);
}

int token_to_ast(Lexer *lexer, TokenType token)
{
    switch (token)
    {
    case TOKEN_PLUS:
        return AST_PLUS;
    case TOKEN_MINUS:
        return AST_MINUS;
    case TOKEN_STAR:
        return AST_STAR;
    case TOKEN_SLASH:
        return AST_SLASH;
    default:
        fprintf(stderr, "[ast.c][token_to_ast][Line %d]: Unknown token '%.*s'.\n", lexer->line, lexer->current_token.length, lexer->current_token.lexeme);
        exit(EXIT_FAILURE);
    }
}

Node *parse_primary(Lexer *lexer)
{
    Token token = lexer->current_token;

    if (token.type == TOKEN_NUMBER)
    {
        scan_token(lexer);
        return make_leaf(AST_NUMBER, atoi(token.lexeme));
    }
    else if (token.type == TOKEN_LPAREN)
    {
        scan_token(lexer);
        Node *node = parse_binary_expression(lexer);
        if (lexer->current_token.type != TOKEN_RPAREN)
        {
            fprintf(stderr, "[ast.c][parse_primary][Line %d]: Expected closing parenthesis.\n", lexer->line);
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);
        return node;
    }
    else
    {
        fprintf(stderr, "[ast.c][parse_primary][Line %d]: Unexpected token '%.*s'.\n", lexer->line, token.length, token.lexeme);
        exit(EXIT_FAILURE);
    }
}

int get_operator_precedence(NodeType type)
{
    switch (type)
    {
    case AST_PLUS:
    case AST_MINUS:
        return 1;
    case AST_STAR:
    case AST_SLASH:
        return 2;
    default:
        return 0;
    }
}

Node *parse_binary_expression_with_precedence(Lexer *lexer, int precedence)
{
    Node *left = parse_primary(lexer);

    while (lexer->current_token.type != TOKEN_EOF)
    {
        int current_precedence = get_operator_precedence(token_to_ast(lexer, lexer->current_token.type));
        if (current_precedence < precedence)
            break;

        NodeType node_type = token_to_ast(lexer, lexer->current_token.type);
        scan_token(lexer);

        Node *right = parse_binary_expression_with_precedence(lexer, current_precedence + 1);
        left = make_node(node_type, left, right, 0);
    }

    return left;
}

Node *parse_binary_expression(Lexer *lexer)
{
    return parse_binary_expression_with_precedence(lexer, 0);
}

void free_ast(Node *node)
{
    if (node == NULL)
        return;

    free_ast(node->left);
    free_ast(node->right);

    free(node);
}