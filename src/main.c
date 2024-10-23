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

    LLVMBuilderRef builder = LLVMCreateBuilder();
    if (!builder)
    {
        fprintf(stderr, "Error: Failed to create LLVM builder.\n");
        LLVMDisposeModule(module);
        free(source);
        exit(EXIT_FAILURE);
    }

    LLVMTypeRef main_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(module, "main", main_type);
    if (!main_func)
    {
        fprintf(stderr, "Error: Failed to add main function to module.\n");
        LLVMDisposeBuilder(builder);
        LLVMDisposeModule(module);
        free(source);
        exit(EXIT_FAILURE);
    }

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    if (!entry)
    {
        fprintf(stderr, "Error: Failed to create entry basic block.\n");
        LLVMDisposeBuilder(builder);
        LLVMDisposeModule(module);
        free(source);
        exit(EXIT_FAILURE);
    }
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMTypeRef printf_type = LLVMFunctionType(
        LLVMInt32Type(),
        (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)},
        1,
        1);

    LLVMValueRef printf_func_llvm = LLVMAddFunction(module, "printf", printf_type);
    if (!printf_func_llvm)
    {
        fprintf(stderr, "Error: Failed to declare printf function.\n");
        LLVMDisposeBuilder(builder);
        LLVMDisposeModule(module);
        free(source);
        exit(EXIT_FAILURE);
    }

    LLVMValueRef format_str = LLVMBuildGlobalStringPtr(builder, "%d\n", "fmt");
    if (!format_str)
    {
        fprintf(stderr, "Error: Failed to create format string.\n");
        LLVMDisposeBuilder(builder);
        LLVMDisposeModule(module);
        free(source);
        exit(EXIT_FAILURE);
    }

    SymbolTable *sym_table = create_symbol_table();

    generate_code(ast, module, builder, printf_func_llvm, format_str, sym_table);
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));

    char *llvm_ir = LLVMPrintModuleToString(module);
    if (!llvm_ir)
    {
        fprintf(stderr, "Error: Failed to print LLVM IR.\n");
        LLVMDisposeBuilder(builder);
        LLVMDisposeModule(module);
        free(source);
        exit(EXIT_FAILURE);
    }
    printf("=== Generated IR ===\n%s\n", llvm_ir);
    LLVMDisposeMessage(llvm_ir);

    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    free_ast(ast);
    free_symbol_table(sym_table);
    free(source);

    return 0;
}
