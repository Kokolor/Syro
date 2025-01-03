// ast.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "ast.h"

Node *make_node(NodeType type, Node *left, Node *right, int number_value)
{
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL)
    {
        error_report(-1, "Memory allocation failed in make_node.\n");
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
    node->condition = NULL;
    node->then_branch = NULL;
    node->else_branch = NULL;
    node->init = NULL;
    node->increment = NULL;

    return node;
}

Node *make_leaf(NodeType type, int number_value)
{
    return make_node(type, NULL, NULL, number_value);
}

Node *make_assignment(char *var_name, Node *expression)
{
    Node *node = make_node(AST_ASSIGNMENT, NULL, NULL, 0);
    node->var_name = var_name;
    node->expression = expression;
    return node;
}

Node *make_dereference_assignment(Node *dereferenced_expr, Node *value_expr)
{
    Node *node = make_node(AST_DEREFERENCE_ASSIGNMENT, dereferenced_expr, value_expr, 0);
    return node;
}

Node *make_array_type(char *element_type, int size)
{
    Node *node = make_node(AST_ARRAY_TYPE, NULL, NULL, 0);
    node->var_type = element_type;
    node->number_value = size;
    return node;
}

Node *make_array_decl(char *var_name, Node *array_type, Node **elements, int element_count)
{
    Node *node = make_node(AST_ARRAY_DECL, NULL, NULL, 0);
    node->var_name = var_name;
    node->var_type = array_type->var_type;
    node->number_value = array_type->number_value;
    node->parameters = elements;
    node->param_count = element_count;
    return node;
}

Node *make_array_access(char *var_name, Node *index)
{
    Node *node = make_node(AST_ARRAY_ACCESS, NULL, NULL, 0);
    node->var_name = var_name;
    node->expression = index;
    return node;
}

Node *make_array_assignment(char *array_name, Node *index, Node *value)
{
    Node *node = make_node(AST_ARRAY_ASSIGNMENT, NULL, NULL, 0);
    node->var_name = array_name;
    node->left = index;
    node->right = value;
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

Node *make_while_statement(Node *condition, Node *body)
{
    Node *node = make_node(AST_WHILE_STATEMENT, NULL, NULL, 0);
    node->condition = condition;
    node->body = body;
    return node;
}

Node *make_for_statement(Node *init, Node *condition, Node *increment, Node *body)
{
    Node *node = make_node(AST_FOR_STATEMENT, NULL, NULL, 0);
    node->init = init;
    node->condition = condition;
    node->increment = increment;
    node->body = body;
    return node;
}

Node *make_address_of(Node *expression)
{
    Node *node = make_node(AST_ADDRESS_OF, NULL, NULL, 0);
    node->expression = expression;
    return node;
}

Node *make_dereference(Node *expression)
{
    Node *node = make_node(AST_DEREFERENCE, NULL, NULL, 0);
    node->expression = expression;
    return node;
}

Node *parse_expression_statement(Lexer *lexer)
{
    if (lexer->current_token.type == TOKEN_IDENTIFIER)
    {
        char *identifier = strndup(lexer->current_token.lexeme, lexer->current_token.length);
        scan_token(lexer);

        if (lexer->current_token.type == TOKEN_EQUAL)
        {
            scan_token(lexer);
            Node *expr = parse_binary_expression(lexer);
            return make_assignment(identifier, expr);
        }
        else if (lexer->current_token.type == TOKEN_LBRACKET)
        {

            scan_token(lexer);
            Node *index = parse_binary_expression(lexer);
            if (lexer->current_token.type != TOKEN_RBRACKET)
            {
                error_report(lexer->line, "Error: Expected ']' after array index.\n");
                exit(EXIT_FAILURE);
            }
            scan_token(lexer);

            if (lexer->current_token.type == TOKEN_EQUAL)
            {
                scan_token(lexer);
                Node *expr = parse_binary_expression(lexer);
                return make_array_assignment(identifier, index, expr);
            }
            else
            {
                error_report(lexer->line, "Error: Expected '=' after array access.\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            error_report(lexer->line, "Error: Expected '=' after identifier '%s' in expression statement.\n", identifier);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        return parse_binary_expression(lexer);
    }
}

Node *parse_unary_expression(Lexer *lexer)
{
    if (lexer->current_token.type == TOKEN_AMPERSAND)
    {
        scan_token(lexer);
        Node *expr = parse_unary_expression(lexer);
        return make_address_of(expr);
    }
    else if (lexer->current_token.type == TOKEN_STAR)
    {
        scan_token(lexer);
        Node *expr = parse_unary_expression(lexer);
        return make_dereference(expr);
    }
    else
    {
        return parse_primary(lexer);
    }
}

Node *parse_binary_expression_with_precedence(Lexer *lexer, int precedence)
{
    Node *left = parse_unary_expression(lexer);

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

char *parse_type(Lexer *lexer)
{
    if (!is_type_token(lexer->current_token.type))
    {
        error_report(lexer->line, "Error: Expected type.\n");
        exit(EXIT_FAILURE);
    }
    char *type_name = strndup(lexer->current_token.lexeme, lexer->current_token.length);
    scan_token(lexer);
    while (lexer->current_token.type == TOKEN_STAR)
    {
        type_name = realloc(type_name, strlen(type_name) + 2);
        strcat(type_name, "*");
        scan_token(lexer);
    }
    if (lexer->current_token.type == TOKEN_LBRACKET)
    {
        scan_token(lexer);
        int size = -1;
        if (lexer->current_token.type == TOKEN_NUMBER)
        {
            size = atoi(lexer->current_token.lexeme);
            scan_token(lexer);
        }
        else
        {
            error_report(lexer->line, "Error: Expected array size.\n");
            exit(EXIT_FAILURE);
        }
        if (lexer->current_token.type != TOKEN_RBRACKET)
        {
            error_report(lexer->line, "Error: Expected ']' after array size.\n");
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);
        char size_str[32];
        sprintf(size_str, "[%d]", size);
        type_name = realloc(type_name, strlen(type_name) + strlen(size_str) + 1);
        strcat(type_name, size_str);
    }
    return type_name;
}

Node *parse_if_statement(Lexer *lexer)
{
    scan_token(lexer);

    if (lexer->current_token.type != TOKEN_LPAREN)
    {
        error_report(lexer->line, "Error: Expected '(' after 'if'.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);

    Node *condition = parse_binary_expression(lexer);

    if (lexer->current_token.type != TOKEN_RPAREN)
    {
        error_report(lexer->line, "Error: Expected ')' after condition.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);

    if (lexer->current_token.type != TOKEN_LBRACE)
    {
        error_report(lexer->line, "Error: Expected '{' after 'if' condition.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);

    Node *then_branch = parse_statement_list(lexer);

    if (lexer->current_token.type != TOKEN_RBRACE)
    {
        error_report(lexer->line, "Error: Expected '}' after 'if' block.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);

    Node *else_branch = NULL;

    if (lexer->current_token.type == TOKEN_ELSE)
    {
        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_LBRACE)
        {
            error_report(lexer->line, "Error: Expected '{' after 'else'.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        else_branch = parse_statement_list(lexer);

        if (lexer->current_token.type != TOKEN_RBRACE)
        {
            error_report(lexer->line, "Error: Expected '}' after 'else' block.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);
    }

    return make_if_statement(condition, then_branch, else_branch);
}

Node *parse_while_statement(Lexer *lexer)
{
    scan_token(lexer);

    if (lexer->current_token.type != TOKEN_LPAREN)
    {
        error_report(lexer->line, "Error: Expected '(' after 'while'.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);
    Node *condition = parse_binary_expression(lexer);

    if (lexer->current_token.type != TOKEN_RPAREN)
    {
        error_report(lexer->line, "Error: Expected ')' after while condition.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);

    if (lexer->current_token.type != TOKEN_LBRACE)
    {
        error_report(lexer->line, "Error: Expected '{' after while condition.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);
    Node *body = parse_statement_list(lexer);

    if (lexer->current_token.type != TOKEN_RBRACE)
    {
        error_report(lexer->line, "Error: Expected '}' after while body.\n");
        exit(EXIT_FAILURE);
    }

    scan_token(lexer);

    return make_while_statement(condition, body);
}

Node *parse_for_statement(Lexer *lexer)
{
    scan_token(lexer);

    if (lexer->current_token.type != TOKEN_LPAREN)
    {
        error_report(lexer->line, "Error: Expected '(' after 'for'.\n");
        exit(EXIT_FAILURE);
    }
    scan_token(lexer);

    Node *init = NULL;
    if (lexer->current_token.type != TOKEN_SEMI)
    {
        init = parse_expression_statement(lexer);
    }
    if (lexer->current_token.type != TOKEN_SEMI)
    {
        error_report(lexer->line, "Error: Expected ';' after 'for' loop initialization.\n");
        exit(EXIT_FAILURE);
    }
    scan_token(lexer);

    Node *condition = NULL;
    if (lexer->current_token.type != TOKEN_SEMI)
    {
        condition = parse_binary_expression(lexer);
    }
    if (lexer->current_token.type != TOKEN_SEMI)
    {
        error_report(lexer->line, "Error: Expected ';' after 'for' loop condition.\n");
        exit(EXIT_FAILURE);
    }
    scan_token(lexer);

    Node *increment = NULL;
    if (lexer->current_token.type != TOKEN_RPAREN)
    {
        increment = parse_expression_statement(lexer);
    }
    if (lexer->current_token.type != TOKEN_RPAREN)
    {
        error_report(lexer->line, "Error: Expected ')' after 'for' loop increment.\n");
        exit(EXIT_FAILURE);
    }
    scan_token(lexer);

    if (lexer->current_token.type != TOKEN_LBRACE)
    {
        error_report(lexer->line, "Error: Expected '{' after 'for' loop header.\n");
        exit(EXIT_FAILURE);
    }
    scan_token(lexer);

    Node *body = parse_statement_list(lexer);

    if (lexer->current_token.type != TOKEN_RBRACE)
    {
        error_report(lexer->line, "Error: Expected '}' after 'for' loop body.\n");
        exit(EXIT_FAILURE);
    }
    scan_token(lexer);

    return make_for_statement(init, condition, increment, body);
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
                    error_report(lexer->line, "Error: Expected ')' after function arguments.\n");
                    exit(EXIT_FAILURE);
                }
            }

            scan_token(lexer);

            return make_function_call(identifier, arguments, arg_count);
        }
        else if (lexer->current_token.type == TOKEN_LBRACKET)
        {
            scan_token(lexer);

            Node *index = parse_binary_expression(lexer);

            if (lexer->current_token.type != TOKEN_RBRACKET)
            {
                error_report(lexer->line, "Error: Expected ']' after array index.\n");
                exit(EXIT_FAILURE);
            }

            scan_token(lexer);

            return make_array_access(identifier, index);
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
            error_report(lexer->line, "Error: Expected ')'.\n");
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);
        return node;
    }
    else if (token.type == TOKEN_PIPE)
    {
        scan_token(lexer);

        char *cast_type = parse_type(lexer);

        if (lexer->current_token.type != TOKEN_PIPE)
        {
            error_report(lexer->line, "Error: Expected '|' after type in cast.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        Node *expr = parse_primary(lexer);
        return make_cast(cast_type, expr);
    }
    else if (token.type == TOKEN_MINUS)
    {
        scan_token(lexer);
        Node *expr = parse_primary(lexer);
        return make_node(AST_NEGATE, expr, NULL, 0);
    }
    else
    {
        error_report(lexer->line, "Error: Unexpected token '%.*s'.\n", token.length, token.lexeme);
        exit(EXIT_FAILURE);
    }
}

Node *parse_statement(Lexer *lexer)
{
    if (lexer->current_token.type == TOKEN_IF)
    {
        return parse_if_statement(lexer);
    }
    else if (lexer->current_token.type == TOKEN_WHILE)
    {
        return parse_while_statement(lexer);
    }
    else if (lexer->current_token.type == TOKEN_FOR)
    {
        return parse_for_statement(lexer);
    }
    else if (lexer->current_token.type == TOKEN_STAR)
    {
        scan_token(lexer);
        Node *dereferenced_expr = parse_unary_expression(lexer);

        if (lexer->current_token.type != TOKEN_EQUAL)
        {
            error_report(lexer->line, "Error: Expected '=' after dereferenced expression.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        Node *value_expr = parse_binary_expression(lexer);

        if (lexer->current_token.type != TOKEN_SEMI)
        {
            error_report(lexer->line, "Error: Expected ';' after dereference assignment.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        return make_dereference_assignment(dereferenced_expr, value_expr);
    }
    else if (lexer->current_token.type == TOKEN_AT)
    {
        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_IDENTIFIER)
        {
            error_report(lexer->line, "Error: Expected function name after '@'.\n");
            exit(EXIT_FAILURE);
        }

        char *func_name = strndup(lexer->current_token.lexeme, lexer->current_token.length);
        scan_token(lexer);

        if (lexer->current_token.type != TOKEN_LPAREN)
        {
            error_report(lexer->line, "Error: Expected '(' after function name.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        Node **parameters = NULL;
        int param_count = 0;

        while (lexer->current_token.type != TOKEN_RPAREN)
        {
            char *param_type = parse_type(lexer);

            if (lexer->current_token.type != TOKEN_COLON)
            {
                error_report(lexer->line, "Error: Expected ':' after parameter type.\n");
                exit(EXIT_FAILURE);
            }

            scan_token(lexer);

            if (lexer->current_token.type != TOKEN_IDENTIFIER)
            {
                error_report(lexer->line, "Error: Expected parameter name after ':'.\n");
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
                error_report(lexer->line, "Error: Expected ',' or ')' in parameter list.\n");
                exit(EXIT_FAILURE);
            }
        }

        scan_token(lexer);

        char *return_type = NULL;
        if (lexer->current_token.type == TOKEN_ARROW)
        {
            scan_token(lexer);

            return_type = parse_type(lexer);
        }

        if (lexer->current_token.type != TOKEN_LBRACE)
        {
            error_report(lexer->line, "Error: Expected '{' to start function body.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        Node *body = parse_statement_list(lexer);

        if (lexer->current_token.type != TOKEN_RBRACE)
        {
            error_report(lexer->line, "Error: Expected '}' to end function body.\n");
            exit(EXIT_FAILURE);
        }

        scan_token(lexer);

        return make_function_decl(func_name, parameters, param_count, return_type, body);
    }
    else if (is_type_token(lexer->current_token.type))
    {
        char *type_name = parse_type(lexer);
        if (lexer->current_token.type != TOKEN_COLON)
        {
            error_report(lexer->line, "Error: Expected ':' after type in variable declaration.\n");
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);
        if (lexer->current_token.type != TOKEN_IDENTIFIER)
        {
            error_report(lexer->line, "Error: Expected variable name after ':'.\n");
            exit(EXIT_FAILURE);
        }
        char *var_name = strndup(lexer->current_token.lexeme, lexer->current_token.length);
        scan_token(lexer);
        Node *expr = NULL;
        if (lexer->current_token.type == TOKEN_EQUAL)
        {
            scan_token(lexer);
            if (lexer->current_token.type == TOKEN_LBRACE)
            {
                scan_token(lexer);
                Node **elements = NULL;
                int element_count = 0;
                while (lexer->current_token.type != TOKEN_RBRACE)
                {
                    Node *element = parse_binary_expression(lexer);
                    elements = realloc(elements, sizeof(Node *) * (element_count + 1));
                    elements[element_count++] = element;
                    if (lexer->current_token.type == TOKEN_COMMA)
                    {
                        scan_token(lexer);
                    }
                    else
                    {
                        break;
                    }
                }
                if (lexer->current_token.type != TOKEN_RBRACE)
                {
                    error_report(lexer->line, "Error: Expected '}' after array elements.\n");
                    exit(EXIT_FAILURE);
                }
                scan_token(lexer);
                Node *array_type_node = make_array_type(type_name, element_count);
                return make_array_decl(var_name, array_type_node, elements, element_count);
            }
            else
            {
                expr = parse_binary_expression(lexer);
            }
        }
        if (lexer->current_token.type != TOKEN_SEMI)
        {
            error_report(lexer->line, "Error: Expected ';' after variable declaration.\n");
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);
        if (strchr(type_name, '[') != NULL)
        {

            char *bracket_pos = strchr(type_name, '[');
            int size = atoi(bracket_pos + 1);
            *bracket_pos = '\0';
            Node *array_type_node = make_array_type(type_name, size);
            return make_array_decl(var_name, array_type_node, NULL, 0);
        }
        else
        {
            return make_variable_decl(type_name, var_name, expr);
        }
    }
    else if (lexer->current_token.type == TOKEN_IDENTIFIER)
    {
        char *identifier = strndup(lexer->current_token.lexeme, lexer->current_token.length);
        scan_token(lexer);

        if (lexer->current_token.type == TOKEN_EQUAL)
        {
            scan_token(lexer);
            Node *expr = parse_binary_expression(lexer);

            if (lexer->current_token.type != TOKEN_SEMI)
            {
                error_report(lexer->line, "Error: Expected ';' after assignment.\n");
                exit(EXIT_FAILURE);
            }

            scan_token(lexer);
            return make_assignment(identifier, expr);
        }
        else if (lexer->current_token.type == TOKEN_LBRACKET)
        {

            scan_token(lexer);
            Node *index = parse_binary_expression(lexer);
            if (lexer->current_token.type != TOKEN_RBRACKET)
            {
                error_report(lexer->line, "Error: Expected ']' after array index.\n");
                exit(EXIT_FAILURE);
            }
            scan_token(lexer);

            if (lexer->current_token.type == TOKEN_EQUAL)
            {
                scan_token(lexer);
                Node *expr = parse_binary_expression(lexer);

                if (lexer->current_token.type != TOKEN_SEMI)
                {
                    error_report(lexer->line, "Error: Expected ';' after assignment.\n");
                    exit(EXIT_FAILURE);
                }

                scan_token(lexer);
                return make_array_assignment(identifier, index, expr);
            }
            else
            {
                error_report(lexer->line, "Error: Expected '=' after array access.\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (lexer->current_token.type == TOKEN_LPAREN)
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
                    error_report(lexer->line, "Error: Expected ')' after function arguments.\n");
                    exit(EXIT_FAILURE);
                }
            }

            scan_token(lexer);

            if (lexer->current_token.type != TOKEN_SEMI)
            {
                error_report(lexer->line, "Error: Expected ';' after function call.\n");
                exit(EXIT_FAILURE);
            }

            scan_token(lexer);
            return make_function_call(identifier, arguments, arg_count);
        }
        else
        {
            error_report(lexer->line, "Error: Unexpected token after identifier '%s'.\n", identifier);
            exit(EXIT_FAILURE);
        }
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
            error_report(lexer->line, "Error: Expected ';' after return statement.\n");
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
            error_report(lexer->line, "Error: Expected ';' after print statement.\n");
            exit(EXIT_FAILURE);
        }
        scan_token(lexer);
        return make_print(expr);
    }
    else
    {
        error_report(lexer->line, "Error: Unknown statement.\n");
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
        error_report(lexer->line, "Error: Unknown token '%.*s'.\n", lexer->current_token.length, lexer->current_token.lexeme);
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
    case AST_ADDRESS_OF:
    case AST_DEREFERENCE:
        free_ast(node->expression);
        break;
    case AST_PRINT:
    case AST_RETURN_STMT:
        free_ast(node->expression);
        break;
    case AST_VARIABLE_DECL:
        free(node->var_type);
        free(node->var_name);
        free_ast(node->expression);
        break;
    case AST_ARRAY_DECL:
        free(node->var_type);
        free(node->var_name);
        for (int i = 0; i < node->param_count; ++i)
        {
            free_ast(node->parameters[i]);
        }
        free(node->parameters);
        break;
    case AST_ARRAY_ASSIGNMENT:
        free(node->var_name);
        free_ast(node->left);
        free_ast(node->right);
        break;
    case AST_IF_STATEMENT:
        free_ast(node->condition);
        free_ast(node->then_branch);
        free_ast(node->else_branch);
        break;
    case AST_WHILE_STATEMENT:
        free_ast(node->condition);
        free_ast(node->body);
        break;
    case AST_FOR_STATEMENT:
        free_ast(node->init);
        free_ast(node->condition);
        free_ast(node->increment);
        free_ast(node->body);
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
