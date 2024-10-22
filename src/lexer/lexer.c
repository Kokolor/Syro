// lexer.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

void init_lexer(Lexer *lexer, char *source)
{
    lexer->start = source;
    lexer->current_position = source;
    lexer->line = 1;
    scan_token(lexer);
}

Token scan_token(Lexer *lexer)
{
    lexer->start = lexer->current_position;

    if (is_at_end(lexer))
    {
        lexer->current_token = make_token(lexer, TOKEN_EOF);
        return lexer->current_token;
    }

    char character = advance(lexer);

    switch (character)
    {
    case '-':
        lexer->current_token = make_token(lexer, TOKEN_MINUS);
        break;
    case '+':
        lexer->current_token = make_token(lexer, TOKEN_PLUS);
        break;
    case '*':
        lexer->current_token = make_token(lexer, TOKEN_STAR);
        break;
    case '=':
        lexer->current_token = make_token(lexer, TOKEN_EQUAL);
        break;
    case ',':
        lexer->current_token = make_token(lexer, TOKEN_COMMA);
        break;
    case ':':
        lexer->current_token = make_token(lexer, TOKEN_COLON);
        break;
    case ';':
        lexer->current_token = make_token(lexer, TOKEN_SEMI);
        break;
    case '(':
        lexer->current_token = make_token(lexer, TOKEN_LPAREN);
        break;
    case ')':
        lexer->current_token = make_token(lexer, TOKEN_RPAREN);
        break;
    case '/':
        lexer->current_token = make_token(lexer, TOKEN_SLASH);
        break;
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
            lexer->current_token = number(lexer);
        }
        else if (isalpha(character) || character == '_')
        {
            while (isalnum(peek(lexer)) || peek(lexer) == '_')
                advance(lexer);

            lexer->current_token = make_token(lexer, TOKEN_IDENTIFIER);
        }
        else
        {
            fprintf(stderr, "[lexer.c][scan_token][Line %d]: Unexpected character '%c'.\n", lexer->line, character);
            exit(EXIT_FAILURE);
        }
    }

    return lexer->current_token;
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
