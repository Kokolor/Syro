// tokens.h

#ifndef TOKENS_H
#define TOKENS_H

typedef enum
{
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_EQUAL,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMI,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_RETURN,
    TOKEN_ARROW,
    TOKEN_AT,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_PRINT,
    TOKEN_I32,
    TOKEN_VOID,
    TOKEN_EOF,
    TOKEN_UNKNOWN,
} TokenType;

typedef struct
{
    TokenType type;
    char *lexeme;
    int length;
    int line;
} Token;

#endif // TOKENS_H
