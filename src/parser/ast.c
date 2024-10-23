// ast.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

Node *make_node(NodeType type, Node *left, Node *right, int number_value)
{
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL)
    {
        fprintf(stderr, "[ast.c][make_node]: Memory allocation failed for a new AST node.\n");
        exit(EXIT_FAILURE);
    }

    node->type = type;
    node->left = left;
    node->right = right;
    node->number_value = number_value;
    node->var_type = NULL;
    node->var_name = NULL;
    node->expression = NULL;

    return node;
}

Node *make_leaf(NodeType type, int number_value)
{
    return make_node(type, NULL, NULL, number_value);
}

Node *make_print(Node *expression)
{
    Node *node = make_node(AST_PRINT, NULL, NULL, 0);
    node->expression = expression;
    return node;
}

Node *make_variable_decl(char *var_type, char *var_name, Node *expression)
{
    Node *node = make_node(AST_VARIABLE_DECL, NULL, NULL, 0);
    node->var_type = var_type;
    node->var_name = var_name;
    node->expression = expression;
    return node;
}

Node *make_variable_ref(char *var_name)
{
    Node *node = make_node(AST_IDENTIFIER, NULL, NULL, 0);
    node->var_name = var_name;
    return node;
}

Node *make_statement_list(Node *list, Node *statement)
{
    if (list == NULL)
        return make_node(AST_STATEMENT_LIST, statement, NULL, 0);

    Node *current = list;
    while (current->right != NULL && current->type == AST_STATEMENT_LIST)
        current = current->right;

    Node *new_list = make_node(AST_STATEMENT_LIST, statement, NULL, 0);
    current->right = new_list;
    return list;
}

Node *parse_primary(Lexer *lexer)
{
    Token token = lexer->current_token;

    if (token.type == TOKEN_NUMBER)
    {
        scan_token(lexer);
        return make_leaf(AST_NUMBER, atoi(token.lexeme));
    }
    else if (token.type == TOKEN_IDENTIFIER)
    {
        char *var_name = strndup(token.lexeme, token.length);
        if (!var_name)
        {
            fprintf(stderr, "[ast.c][parse_primary][Line %d]: Memory allocation failed for variable name.\n", lexer->line);
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);
        return make_variable_ref(var_name);
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
        fprintf(stderr, "[ast.c][parse_primary][Line %d]: Unexpected token '%.*s'.\n",
                lexer->line, token.length, token.lexeme);
        exit(EXIT_FAILURE);
    }
}

Node *parse_binary_expression_with_precedence(Lexer *lexer, int precedence)
{
    Node *left = parse_primary(lexer);

    while (is_operator(lexer->current_token.type))
    {
        NodeType op_type = token_to_ast(lexer, lexer->current_token.type);
        int current_precedence = get_operator_precedence(op_type);
        if (current_precedence < precedence)
            break;

        scan_token(lexer);

        Node *right = parse_binary_expression_with_precedence(lexer, current_precedence + 1);
        left = make_node(op_type, left, right, 0);
    }

    return left;
}

Node *parse_binary_expression(Lexer *lexer)
{
    return parse_binary_expression_with_precedence(lexer, 0);
}

Node *parse_statement(Lexer *lexer)
{
    if (lexer->current_token.type == TOKEN_PRINT)
    {
        scan_token(lexer);

        Node *expr = parse_binary_expression(lexer);

        if (lexer->current_token.type != TOKEN_SEMI)
        {
            fprintf(stderr, "[ast.c][parse_statement][Line %d]: Expected ';' after print statement.\n", lexer->line);
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);

        return make_print(expr);
    }
    else if (lexer->current_token.type == TOKEN_I32)
    {
        char *var_type = strndup(lexer->current_token.lexeme, lexer->current_token.length);
        if (!var_type)
        {
            fprintf(stderr, "[ast.c][parse_statement][Line %d]: Memory allocation failed for variable type.\n", lexer->line);
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_IDENTIFIER)
        {
            fprintf(stderr, "[ast.c][parse_statement][Line %d]: Expected variable name after type.\n", lexer->line);
            exit(EXIT_FAILURE);
        }

        char *var_name = strndup(lexer->current_token.lexeme, lexer->current_token.length);
        if (!var_name)
        {
            fprintf(stderr, "[ast.c][parse_statement][Line %d]: Memory allocation failed for variable name.\n", lexer->line);
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_EQUAL)
        {
            fprintf(stderr, "[ast.c][parse_statement][Line %d]: Expected '=' after variable name.\n", lexer->line);
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        Node *expr = parse_binary_expression(lexer);

        if (lexer->current_token.type != TOKEN_SEMI)
        {
            fprintf(stderr, "[ast.c][parse_statement][Line %d]: Expected ';' after variable declaration.\n", lexer->line);
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        return make_variable_decl(var_type, var_name, expr);
    }
    else
    {
        fprintf(stderr, "[ast.c][parse_statement][Line %d]: Unknown statement.\n", lexer->line);
        exit(EXIT_FAILURE);
    }
}

Node *parse_statement_list(Lexer *lexer)
{
    Node *list = NULL;

    while (lexer->current_token.type != TOKEN_EOF)
    {
        Node *stmt = parse_statement(lexer);
        list = make_statement_list(list, stmt);
    }

    return list;
}

NodeType token_to_ast(Lexer *lexer, TokenType token)
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
        fprintf(stderr, "[ast.c][token_to_ast][Line %d]: Unknown token '%.*s'.\n",
                lexer->line, lexer->current_token.length, lexer->current_token.lexeme);
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

int is_operator(TokenType token)
{
    return token == TOKEN_PLUS || token == TOKEN_MINUS ||
           token == TOKEN_STAR || token == TOKEN_SLASH;
}

void free_ast(Node *node)
{
    if (node == NULL)
        return;

    if (node->type == AST_PRINT)
        free_ast(node->expression);
    else if (node->type == AST_VARIABLE_DECL)
    {
        free(node->var_type);
        free(node->var_name);
        free_ast(node->expression);
    }
    else if (node->type == AST_IDENTIFIER)
        free(node->var_name);
    else
    {
        free_ast(node->left);
        free_ast(node->right);
    }

    free(node);
}
