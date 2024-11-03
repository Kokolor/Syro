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
        fprintf(stderr, "Error: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    node->type = type;
    node->left = left;
    node->right = right;
    node->number_value = number_value;
    node->var_type = NULL;
    node->var_name = NULL;
    node->expression = NULL;
    node->func_name = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->body = NULL;
    node->return_type = NULL;
    node->cast_type = NULL;
    return node;
}

Node *make_function_decl(char *func_name, Node **parameters, int param_count, char *return_type, Node *body)
{
    Node *node = make_node(AST_FUNCTION_DECL, NULL, NULL, 0);
    node->func_name = func_name;
    node->parameters = parameters;
    node->param_count = param_count;
    node->return_type = return_type;
    node->body = body;
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

Node *make_return_stmt(Node *expression)
{
    Node *node = make_node(AST_RETURN_STMT, NULL, NULL, 0);
    node->expression = expression;
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

Node *make_variable_ref(char *var_name)
{
    Node *node = make_node(AST_IDENTIFIER, NULL, NULL, 0);
    node->var_name = var_name;
    return node;
}

Node *make_function_call(char *func_name, Node **arguments, int arg_count)
{
    Node *node = make_node(AST_FUNCTION_CALL, NULL, NULL, 0);
    node->func_name = func_name;
    node->parameters = arguments;
    node->param_count = arg_count;
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

Node *make_cast(char *cast_type, Node *expression)
{
    Node *node = make_node(AST_CAST, NULL, NULL, 0);
    node->cast_type = cast_type;
    node->expression = expression;
    return node;
}

Node *make_if_statement(Node *condition, Node *then_branch, Node *else_branch)
{
    Node *node = make_node(AST_IF_STATEMENT, NULL, NULL, 0);
    node->condition = condition;
    node->then_branch = then_branch;
    node->else_branch = else_branch;
    return node;
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
        char *identifier = strndup(token.lexeme, token.length);
        scan_token(lexer);

        if (lexer->current_token.type == TOKEN_LPAREN)
        {
            scan_token(lexer);

            Node **arguments = NULL;
            int arg_count = 0;

            if (lexer->current_token.type != TOKEN_RPAREN)
            {
                do
                {
                    Node *arg = parse_binary_expression(lexer);
                    arguments = realloc(arguments, sizeof(Node *) * (arg_count + 1));
                    arguments[arg_count++] = arg;

                    if (lexer->current_token.type == TOKEN_COMMA)
                    {
                        scan_token(lexer);
                    }
                    else
                    {
                        break;
                    }
                } while (lexer->current_token.type != TOKEN_RPAREN);

                if (lexer->current_token.type != TOKEN_RPAREN)
                {
                    fprintf(stderr, "Error: Expected ')' after function arguments.\n");
                    exit(EXIT_FAILURE);
                }
            }

            scan_token(lexer);

            return make_function_call(identifier, arguments, arg_count);
        }
        else
        {
            return make_variable_ref(identifier);
        }
    }
    else if (token.type == TOKEN_LPAREN)
    {
        scan_token(lexer);
        Node *node = parse_binary_expression(lexer);
        if (lexer->current_token.type != TOKEN_RPAREN)
        {
            fprintf(stderr, "Error: Expected ')'.\n");
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);
        return node;
    }
    else if (token.type == TOKEN_PIPE)
    {
        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_IDENTIFIER && !is_type_token(lexer->current_token.type))
        {
            fprintf(stderr, "Error: Expected type after '|'.\n");
            exit(EXIT_FAILURE);
        }

        char *cast_type = strndup(lexer->current_token.lexeme, lexer->current_token.length);
        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_PIPE)
        {
            fprintf(stderr, "Error: Expected '|' after type in cast.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        Node *expr = parse_primary(lexer);
        return make_cast(cast_type, expr);
    }
    else
    {
        fprintf(stderr, "Error: Unexpected token '%.*s'.\n", token.length, token.lexeme);
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

Node *parse_if_statement(Lexer *lexer)
{
    scan_token(lexer);

    if (lexer->current_token.type != TOKEN_LPAREN)
    {
        fprintf(stderr, "Error: Expected '(' after 'if'.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);

    Node *condition = parse_binary_expression(lexer);

    if (lexer->current_token.type != TOKEN_RPAREN)
    {
        fprintf(stderr, "Error: Expected ')' after condition.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);

    if (lexer->current_token.type != TOKEN_LBRACE)
    {
        fprintf(stderr, "Error: Expected '{' after 'if' condition.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);

    Node *then_branch = parse_statement_list(lexer);

    if (lexer->current_token.type != TOKEN_RBRACE)
    {
        fprintf(stderr, "Error: Expected '}' after 'if' block.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);

    Node *else_branch = NULL;

    if (lexer->current_token.type == TOKEN_ELSE)
    {
        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_LBRACE)
        {
            fprintf(stderr, "Error: Expected '{' after 'else'.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        else_branch = parse_statement_list(lexer);

        if (lexer->current_token.type != TOKEN_RBRACE)
        {
            fprintf(stderr, "Error: Expected '}' after 'else' block.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);
    }

    return make_if_statement(condition, then_branch, else_branch);
}

Node *parse_statement(Lexer *lexer)
{
    if (lexer->current_token.type == TOKEN_IF)
    {
        return parse_if_statement(lexer);
    }
    if (lexer->current_token.type == TOKEN_AT)
    {
        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_IDENTIFIER)
        {
            fprintf(stderr, "Error: Expected function name after '@'.\n");
            exit(EXIT_FAILURE);
        }

        char *func_name = strndup(lexer->current_token.lexeme, lexer->current_token.length);
        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_LPAREN)
        {
            fprintf(stderr, "Error: Expected '(' after function name.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        Node **parameters = NULL;
        int param_count = 0;

        while (lexer->current_token.type != TOKEN_RPAREN)
        {
            if (!is_type_token(lexer->current_token.type) && lexer->current_token.type != TOKEN_IDENTIFIER)
            {
                fprintf(stderr, "Error: Expected type in parameter list.\n");
                exit(EXIT_FAILURE);
            }

            char *param_type = strndup(lexer->current_token.lexeme, lexer->current_token.length);
            scan_token(lexer);

            if (lexer->current_token.type != TOKEN_COLON)
            {
                fprintf(stderr, "Error: Expected ':' after parameter type.\n");
                exit(EXIT_FAILURE);
            }

            scan_token(lexer);

            if (lexer->current_token.type != TOKEN_IDENTIFIER)
            {
                fprintf(stderr, "Error: Expected parameter name after ':'.\n");
                exit(EXIT_FAILURE);
            }

            char *param_name = strndup(lexer->current_token.lexeme, lexer->current_token.length);
            scan_token(lexer);

            Node *param = make_variable_decl(param_type, param_name, NULL);

            parameters = realloc(parameters, sizeof(Node *) * (param_count + 1));
            parameters[param_count++] = param;

            if (lexer->current_token.type == TOKEN_COMMA)
            {
                scan_token(lexer);
            }
            else if (lexer->current_token.type != TOKEN_RPAREN)
            {
                fprintf(stderr, "Error: Expected ',' or ')' in parameter list.\n");
                exit(EXIT_FAILURE);
            }
        }

        scan_token(lexer);

        char *return_type = NULL;
        if (lexer->current_token.type == TOKEN_ARROW)
        {
            scan_token(lexer);

            if (!is_type_token(lexer->current_token.type) && lexer->current_token.type != TOKEN_IDENTIFIER)
            {
                fprintf(stderr, "Error: Expected return type after '->'.\n");
                exit(EXIT_FAILURE);
            }

            return_type = strndup(lexer->current_token.lexeme, lexer->current_token.length);
            scan_token(lexer);
        }

        if (lexer->current_token.type != TOKEN_LBRACE)
        {
            fprintf(stderr, "Error: Expected '{' to start function body.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        Node *body = parse_statement_list(lexer);

        if (lexer->current_token.type != TOKEN_RBRACE)
        {
            fprintf(stderr, "Error: Expected '}' to end function body.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        return make_function_decl(func_name, parameters, param_count, return_type, body);
    }
    else if (is_type_token(lexer->current_token.type) || lexer->current_token.type == TOKEN_IDENTIFIER)
    {
        char *var_type = strndup(lexer->current_token.lexeme, lexer->current_token.length);
        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_COLON)
        {
            fprintf(stderr, "Error: Expected ':' after variable type.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_IDENTIFIER)
        {
            fprintf(stderr, "Error: Expected variable name after ':'.\n");
            exit(EXIT_FAILURE);
        }

        char *var_name = strndup(lexer->current_token.lexeme, lexer->current_token.length);
        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_EQUAL)
        {
            fprintf(stderr, "Error: Expected '=' after variable name.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        Node *expr = parse_binary_expression(lexer);

        if (lexer->current_token.type != TOKEN_SEMI)
        {
            fprintf(stderr, "Error: Expected ';' after variable declaration.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        return make_variable_decl(var_type, var_name, expr);
    }
    else if (lexer->current_token.type == TOKEN_RETURN)
    {
        scan_token(lexer);
        Node *expr = NULL;
        if (lexer->current_token.type != TOKEN_SEMI)
        {
            expr = parse_binary_expression(lexer);
        }
        if (lexer->current_token.type != TOKEN_SEMI)
        {
            fprintf(stderr, "Error: Expected ';' after return statement.\n");
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);
        return make_return_stmt(expr);
    }
    else if (lexer->current_token.type == TOKEN_PRINT)
    {
        scan_token(lexer);
        Node *expr = parse_binary_expression(lexer);
        if (lexer->current_token.type != TOKEN_SEMI)
        {
            fprintf(stderr, "Error: Expected ';' after print statement.\n");
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);
        return make_print(expr);
    }
    else
    {
        fprintf(stderr, "Error: Unknown statement.\n");
        exit(EXIT_FAILURE);
    }
}

Node *parse_statement_list(Lexer *lexer)
{
    Node *list = NULL;

    while (lexer->current_token.type != TOKEN_EOF &&
           lexer->current_token.type != TOKEN_RBRACE)
    {
        Node *stmt = parse_statement(lexer);
        list = make_statement_list(list, stmt);
    }

    return list;
}

int is_type_token(TokenType token)
{
    return token == TOKEN_I8 || token == TOKEN_I16 || token == TOKEN_I32 ||
           token == TOKEN_I64 || token == TOKEN_VOID;
}

int is_operator(TokenType token)
{
    return token == TOKEN_PLUS || token == TOKEN_MINUS ||
           token == TOKEN_STAR || token == TOKEN_SLASH ||
           token == TOKEN_EQUAL_EQUAL || token == TOKEN_BANG_EQUAL ||
           token == TOKEN_LESS || token == TOKEN_LESS_EQUAL ||
           token == TOKEN_GREATER || token == TOKEN_GREATER_EQUAL;
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
    case TOKEN_EQUAL_EQUAL:
        return AST_EQUAL_EQUAL;
    case TOKEN_BANG_EQUAL:
        return AST_BANG_EQUAL;
    case TOKEN_LESS:
        return AST_LESS;
    case TOKEN_LESS_EQUAL:
        return AST_LESS_EQUAL;
    case TOKEN_GREATER:
        return AST_GREATER;
    case TOKEN_GREATER_EQUAL:
        return AST_GREATER_EQUAL;
    default:
        fprintf(stderr, "Error: Unknown token '%.*s'.\n", lexer->current_token.length, lexer->current_token.lexeme);
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
    case AST_EQUAL_EQUAL:
    case AST_BANG_EQUAL:
    case AST_LESS:
    case AST_LESS_EQUAL:
    case AST_GREATER:
    case AST_GREATER_EQUAL:
        return 0;
    default:
        return -1;
    }
}

void free_ast(Node *node)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case AST_PRINT:
    case AST_RETURN_STMT:
        free_ast(node->expression);
        break;
    case AST_VARIABLE_DECL:
        free(node->var_type);
        free(node->var_name);
        free_ast(node->expression);
        break;
    case AST_IF_STATEMENT:
        free_ast(node->condition);
        free_ast(node->then_branch);
        free_ast(node->else_branch);
        break;
    case AST_IDENTIFIER:
        free(node->var_name);
        break;
    case AST_FUNCTION_CALL:
        free(node->func_name);
        for (int i = 0; i < node->param_count; ++i)
        {
            free_ast(node->parameters[i]);
        }
        free(node->parameters);
        break;
    case AST_FUNCTION_DECL:
        free(node->func_name);
        for (int i = 0; i < node->param_count; ++i)
        {
            free_ast(node->parameters[i]);
        }
        free(node->parameters);
        free(node->return_type);
        free_ast(node->body);
        break;
    case AST_STATEMENT_LIST:
        free_ast(node->left);
        free_ast(node->right);
        break;
    case AST_CAST:
        free(node->cast_type);
        free_ast(node->expression);
        break;
    default:
        free_ast(node->left);
        free_ast(node->right);
        break;
    }

    free(node);
}
