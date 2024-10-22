// main.c

#include <stdio.h>
#include "lexer/lexer.h"

char *get_token_type_name(TokenType type)
{
    switch (type)
    {
    case TOKEN_MINUS:
        return "TOKEN_MINUS";
    case TOKEN_PLUS:
        return "TOKEN_PLUS";
    case TOKEN_SLASH:
        return "TOKEN_SLASH";
    case TOKEN_STAR:
        return "TOKEN_STAR";
    case TOKEN_COMMA:
        return "TOKEN_COMMA";
    case TOKEN_COLON:
        return "TOKEN_COLON";
    case TOKEN_SEMI:
        return "TOKEN_SEMI";
    case TOKEN_IDENTIFIER:
        return "TOKEN_IDENTIFIER";
    case TOKEN_NUMBER:
        return "TOKEN_NUMBER";
    case TOKEN_LPAREN:
        return "TOKEN_LPAREN";
    case TOKEN_RPAREN:
        return "TOKEN_RPAREN";
    case TOKEN_EOF:
        return "TOKEN_EOF";
    case TOKEN_UNKNOWN:
        return "TOKEN_UNKNOWN";
    default:
        return "UNKNOWN_TOKEN";
    }
}

int main()
{
    char source[] = "test = 4 * (3 - 2);";
    Lexer lexer;
    init_lexer(&lexer, source);

    Token token;

    while (1)
    {
        token = scan_token(&lexer);

        if (token.type == TOKEN_UNKNOWN)
        {
            printf("[Line %d] Error: %.*s\n", token.line, token.length, token.lexeme);
            continue;
        }

        printf("Token Type: %s, Lexeme: %.*s, Line: %d\n",
               get_token_type_name(token.type),
               token.length,
               token.lexeme,
               token.line);

        if (token.type == TOKEN_EOF)
        {
            break;
        }
    }

    return 0;
}