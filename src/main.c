// main.c

#include <stdio.h>
#include <stdlib.h>
#include "codegen/codegen.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "symbol_table/symbol_table.h"

int main()
{
    FILE *file = fopen("main.syro", "r");
    if (!file)
    {
        fprintf(stderr, "Error: Could not open file main.syro.\n");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *source = (char *)malloc(file_size + 1);
    if (!source)
    {
        fprintf(stderr, "Error: Failed to allocate memory for source code.\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);

    Lexer lexer;
    init_lexer(&lexer, source);

    Node *ast = parse_statement_list(&lexer);
    if (!ast)
    {
        fprintf(stderr, "Error: Failed to parse AST.\n");
        free(source);
        exit(EXIT_FAILURE);
    }

    LLVMModuleRef module = LLVMModuleCreateWithName("module");
    if (!module)
    {
        fprintf(stderr, "Error: Failed to create LLVM module.\n");
        free(source);
        exit(EXIT_FAILURE);
    }

    LLVMTypeRef printf_type = LLVMFunctionType(
        LLVMInt32Type(),
        (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)},
        1,
        1);

    LLVMValueRef printf_func_llvm = LLVMAddFunction(module, "printf", printf_type);
    if (!printf_func_llvm)
    {
        fprintf(stderr, "Error: Failed to declare printf function.\n");
        LLVMDisposeModule(module);
        free(source);
        exit(EXIT_FAILURE);
    }

    LLVMValueRef format_str = LLVMAddGlobal(module, LLVMArrayType(LLVMInt8Type(), 4), "fmt");
    LLVMSetInitializer(format_str, LLVMConstString("%d\n", 4, 1));
    LLVMSetGlobalConstant(format_str, 1);
    LLVMSetLinkage(format_str, LLVMPrivateLinkage);

    SymbolTable *sym_table = create_symbol_table();

    generate_code(ast, module, printf_func_llvm, format_str, sym_table, NULL);

    LLVMValueRef main_func = LLVMGetNamedFunction(module, "main");
    if (!main_func)
    {
        fprintf(stderr, "Error: No 'main' function defined in syro code.\n");
        LLVMDisposeModule(module);
        free_ast(ast);
        free_symbol_table(sym_table);
        free(source);
        exit(EXIT_FAILURE);
    }

    char *llvm_ir = LLVMPrintModuleToString(module);
    if (!llvm_ir)
    {
        fprintf(stderr, "Error: Failed to print LLVM IR.\n");
        LLVMDisposeModule(module);
        free_ast(ast);
        free_symbol_table(sym_table);
        free(source);
        exit(EXIT_FAILURE);
    }
    printf("%s", llvm_ir);
    LLVMDisposeMessage(llvm_ir);

    LLVMDisposeModule(module);
    free_ast(ast);
    free_symbol_table(sym_table);
    free(source);

    return 0;
}
