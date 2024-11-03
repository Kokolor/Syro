// lexer.h

#ifndef LEXER_H
#define LEXER_H

#include "tokens.h"

typedef struct
{
    char *start;
    char *current_position;
    int line;
    Token current_token;
} Lexer;

void init_lexer(Lexer *lexer, char *source);
Token scan_token(Lexer *lexer);
Token make_token(Lexer *lexer, TokenType type);
Token number(Lexer *lexer);
char advance(Lexer *lexer);
char peek(Lexer *lexer);
int is_at_end(Lexer *lexer);

#endif // LEXER_H
