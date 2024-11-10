// symbol_table.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"
#include "error.h"

SymbolTable *create_symbol_table()
{
    SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));
    if (!table)
    {
        error_report(-1, "Memory allocation failed in create_symbol_table.\n");
        exit(EXIT_FAILURE);
    }
    table->head = NULL;
    return table;
}

void add_symbol(SymbolTable *table, char *name, LLVMValueRef value)
{
    if (get_symbol(table, name))
    {
        error_report(-1, "Symbol '%s' already defined.\n", name);
        exit(EXIT_FAILURE);
    }

    Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
    if (!symbol)
    {
        error_report(-1, "Memory allocation failed in add_symbol.\n");
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
