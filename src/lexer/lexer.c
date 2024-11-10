// lexer.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "error.h"

void init_lexer(Lexer *lexer, char *source)
{
    lexer->start = source;
    lexer->current_position = source;
    lexer->line = 1;
    scan_token(lexer);
}

TokenType check_keyword(char *start, int length)
{
    if (length == 2 && strncmp(start, "i8", 2) == 0)
        return TOKEN_I8;
    if (length == 3 && strncmp(start, "i16", 3) == 0)
        return TOKEN_I16;
    if (length == 3 && strncmp(start, "i32", 3) == 0)
        return TOKEN_I32;
    if (length == 3 && strncmp(start, "i64", 3) == 0)
        return TOKEN_I64;
    if (length == 4 && strncmp(start, "void", 4) == 0)
        return TOKEN_VOID;
    if (length == 6 && strncmp(start, "return", 6) == 0)
        return TOKEN_RETURN;
    if (length == 5 && strncmp(start, "print", 5) == 0)
        return TOKEN_PRINT;
    if (length == 2 && strncmp(start, "if", 2) == 0)
        return TOKEN_IF;
    if (length == 4 && strncmp(start, "else", 4) == 0)
        return TOKEN_ELSE;
    if (length == 5 && strncmp(start, "while", 5) == 0)
        return TOKEN_WHILE;

    return TOKEN_IDENTIFIER;
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

char advance(Lexer *lexer)
{
    lexer->current_position++;
    return lexer->current_position[-1];
}

char peek(Lexer *lexer)
{
    return *lexer->current_position;
}

char peek_next(Lexer *lexer)
{
    if (is_at_end(lexer))
        return '\0';
    return lexer->current_position[1];
}

int is_at_end(Lexer *lexer)
{
    return *lexer->current_position == '\0';
}

void skip_whitespace(Lexer *lexer)
{
    for (;;)
    {
        char c = peek(lexer);
        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            advance(lexer);
            break;
        case '\n':
            lexer->line++;
            advance(lexer);
            break;
        default:
            return;
        }
    }
}

Token scan_token(Lexer *lexer)
{
    skip_whitespace(lexer);

    lexer->start = lexer->current_position;

    if (is_at_end(lexer))
    {
        lexer->current_token = make_token(lexer, TOKEN_EOF);
        return lexer->current_token;
    }

    char c = advance(lexer);

    if (isdigit(c))
    {
        while (isdigit(peek(lexer)))
            advance(lexer);

        lexer->current_token = make_token(lexer, TOKEN_NUMBER);
        return lexer->current_token;
    }

    if (isalpha(c) || c == '_')
    {
        while (isalnum(peek(lexer)) || peek(lexer) == '_')
            advance(lexer);

        int length = (int)(lexer->current_position - lexer->start);
        lexer->current_token = make_token(lexer, check_keyword(lexer->start, length));
        return lexer->current_token;
    }

    switch (c)
    {
    case '+':
        lexer->current_token = make_token(lexer, TOKEN_PLUS);
        break;
    case '-':
        if (peek(lexer) == '>')
        {
            advance(lexer);
            lexer->current_token = make_token(lexer, TOKEN_ARROW);
        }
        else
        {
            lexer->current_token = make_token(lexer, TOKEN_MINUS);
        }
        break;
    case '*':
        lexer->current_token = make_token(lexer, TOKEN_STAR);
        break;
    case '/':
        lexer->current_token = make_token(lexer, TOKEN_SLASH);
        break;
    case '&':
        lexer->current_token = make_token(lexer, TOKEN_AMPERSAND);
        break;
    case '=':
        if (peek(lexer) == '=')
        {
            advance(lexer);
            lexer->current_token = make_token(lexer, TOKEN_EQUAL_EQUAL);
        }
        else
        {
            lexer->current_token = make_token(lexer, TOKEN_EQUAL);
        }
        break;
    case '!':
        if (peek(lexer) == '=')
        {
            advance(lexer);
            lexer->current_token = make_token(lexer, TOKEN_BANG_EQUAL);
        }
        else
        {
            error_report(lexer->line, "Unexpected character '!'");
            exit(EXIT_FAILURE);
        }
        break;
    case '<':
        if (peek(lexer) == '=')
        {
            advance(lexer);
            lexer->current_token = make_token(lexer, TOKEN_LESS_EQUAL);
        }
        else
        {
            lexer->current_token = make_token(lexer, TOKEN_LESS);
        }
        break;
    case '>':
        if (peek(lexer) == '=')
        {
            advance(lexer);
            lexer->current_token = make_token(lexer, TOKEN_GREATER_EQUAL);
        }
        else
        {
            lexer->current_token = make_token(lexer, TOKEN_GREATER);
        }
        break;
    case '(':
        lexer->current_token = make_token(lexer, TOKEN_LPAREN);
        break;
    case ')':
        lexer->current_token = make_token(lexer, TOKEN_RPAREN);
        break;
    case '{':
        lexer->current_token = make_token(lexer, TOKEN_LBRACE);
        break;
    case '}':
        lexer->current_token = make_token(lexer, TOKEN_RBRACE);
        break;
    case ',':
        lexer->current_token = make_token(lexer, TOKEN_COMMA);
        break;
    case ';':
        lexer->current_token = make_token(lexer, TOKEN_SEMI);
        break;
    case ':':
        lexer->current_token = make_token(lexer, TOKEN_COLON);
        break;
    case '@':
        lexer->current_token = make_token(lexer, TOKEN_AT);
        break;
    case '|':
        lexer->current_token = make_token(lexer, TOKEN_PIPE);
        break;
    default:
        error_report(lexer->line, "Unexpected character '%c'", c);
        exit(EXIT_FAILURE);
    }

    return lexer->current_token;
}
