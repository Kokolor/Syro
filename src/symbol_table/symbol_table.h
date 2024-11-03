// symbol_table.h

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <llvm-c/Core.h>

typedef struct Symbol
{
    char *name;
    LLVMValueRef value;
    struct Symbol *next;
} Symbol;

typedef struct
{
    Symbol *head;
} SymbolTable;

SymbolTable *create_symbol_table();

void add_symbol(SymbolTable *table, char *name, LLVMValueRef value);

LLVMValueRef get_symbol(SymbolTable *table, char *name);

void free_symbol_table(SymbolTable *table);

#endif // SYMBOL_TABLE_H
