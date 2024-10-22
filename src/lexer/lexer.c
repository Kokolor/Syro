// lexer.c

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

void init_lexer(Lexer *lexer, char *source)
{
    lexer->start = source;
    lexer->current_position = source;
    lexer->line = 1;
}

Token scan_token(Lexer *lexer)
{
    lexer->start = lexer->current_position;

    if (is_at_end(lexer))
        return make_token(lexer, TOKEN_EOF);

    char character = advance(lexer);

    switch (character)
    {
    case '-':
        return make_token(lexer, TOKEN_MINUS);
    case '+':
        return make_token(lexer, TOKEN_PLUS);
    case '*':
        return make_token(lexer, TOKEN_STAR);
    case '=':
        return make_token(lexer, TOKEN_EQUAL);
    case ',':
        return make_token(lexer, TOKEN_COMMA);
    case ':':
        return make_token(lexer, TOKEN_COLON);
    case ';':
        return make_token(lexer, TOKEN_SEMI);
    case '(':
        return make_token(lexer, TOKEN_LPAREN);
    case ')':
        return make_token(lexer, TOKEN_RPAREN);
    case ' ':
    case '\r':
    case '\t':
        return scan_token(lexer);
    case '\n':
        lexer->line++;
        return scan_token(lexer);
    default:
        if (isdigit(character))
        {
            return number(lexer);
        }

        if (isalpha(character) || character == '_')
        {
            while (isalnum(peek(lexer)) || peek(lexer) == '_')
                advance(lexer);

            return make_token(lexer, TOKEN_IDENTIFIER);
        }

        return make_error_token(lexer, "Unexpected character.");
    }
}

Token make_error_token(Lexer *lexer, char *message)
{
    Token token;
    token.type = TOKEN_UNKNOWN;
    token.lexeme = strdup(message);
    token.length = (int)strlen(message);
    token.line = lexer->line;

    return token;
}

Token make_token(Lexer *lexer, TokenType type)
{
    Token token;
    token.type = type;
    token.lexeme = lexer->start;
    token.length = (int)(lexer->current_position - lexer->start);
    token.line = lexer->line;
    return token;
}

Token number(Lexer *lexer)
{
    while (isdigit(peek(lexer)))
        advance(lexer);

    return make_token(lexer, TOKEN_NUMBER);
}

char advance(Lexer *lexer)
{
    lexer->current_position++;
    return lexer->current_position[-1];
}

char peek(Lexer *lexer)
{
    return *lexer->current_position;
}

int is_at_end(Lexer *lexer)
{
    return *lexer->current_position == '\0';
}
