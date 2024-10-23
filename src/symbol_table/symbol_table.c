#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

SymbolTable *create_symbol_table()
{
    SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));
    if (!table)
    {
        fprintf(stderr, "[symbol_table.c][create_symbol_table]: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    table->head = NULL;
    return table;
}

void add_symbol(SymbolTable *table, char *name, LLVMValueRef value)
{
    Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
    if (!symbol)
    {
        fprintf(stderr, "[symbol_table.c][add_symbol]: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    symbol->name = strdup(name);
    symbol->value = value;
    symbol->next = table->head;
    table->head = symbol;
}

LLVMValueRef get_symbol(SymbolTable *table, char *name)
{
    Symbol *current = table->head;
    while (current)
    {
        if (strcmp(current->name, name) == 0)
            return current->value;
        current = current->next;
    }
    return NULL;
}

void free_symbol_table(SymbolTable *table)
{
    Symbol *current = table->head;
    while (current)
    {
        Symbol *temp = current;
        current = current->next;
        free(temp->name);
        free(temp);
    }
    free(table);
}
