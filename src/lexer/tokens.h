// tokens.h

#ifndef TOKENS_H
#define TOKENS_H

typedef enum
{
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_EQUAL,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_SEMI,
    TOKEN_COLON,
    TOKEN_ARROW,
    TOKEN_AT,
    TOKEN_PIPE,

    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,

    TOKEN_I8,
    TOKEN_I16,
    TOKEN_I32,
    TOKEN_I64,
    TOKEN_VOID,
    TOKEN_RETURN,
    TOKEN_PRINT,

    TOKEN_EOF
} TokenType;

typedef struct
{
    TokenType type;
    char *lexeme;
    int length;
    int line;
} Token;

#endif // TOKENS_H
