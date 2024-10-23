// main.c

#include <stdio.h>
#include <stdlib.h>
#include "codegen/codegen.h"
#include "lexer/lexer.h"
#include "parser/ast.h"

int main()
{
    char source[] = "print 4 + 3; print 7 - 2;\n";
    Lexer lexer;
    init_lexer(&lexer, source);

    Node *ast = parse_statement_list(&lexer);
    if (!ast)
    {
        fprintf(stderr, "Error: Failed to parse AST.\n");
        exit(EXIT_FAILURE);
    }

    LLVMModuleRef module = LLVMModuleCreateWithName("module");
    if (!module)
    {
        fprintf(stderr, "Error: Failed to create LLVM module.\n");
        exit(EXIT_FAILURE);
    }

    LLVMBuilderRef builder = LLVMCreateBuilder();
    if (!builder)
    {
        fprintf(stderr, "Error: Failed to create LLVM builder.\n");
        exit(EXIT_FAILURE);
    }

    LLVMTypeRef main_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(module, "main", main_type);
    if (!main_func)
    {
        fprintf(stderr, "Error: Failed to add main function to module.\n");
        exit(EXIT_FAILURE);
    }

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    if (!entry)
    {
        fprintf(stderr, "Error: Failed to create entry basic block.\n");
        exit(EXIT_FAILURE);
    }
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMTypeRef printf_type = LLVMFunctionType(
        LLVMInt32Type(),
        (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)},
        1,
        1);

    LLVMValueRef printf_func = LLVMAddFunction(module, "printf", printf_type);
    if (!printf_func)
    {
        fprintf(stderr, "Error: Failed to declare printf function.\n");
        exit(EXIT_FAILURE);
    }

    LLVMValueRef format_str = LLVMBuildGlobalStringPtr(builder, "%d\n", "fmt");
    if (!format_str)
    {
        fprintf(stderr, "Error: Failed to create format string.\n");
        exit(EXIT_FAILURE);
    }

    generate_code(ast, module, builder, printf_func, format_str);
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32Type(), 0, 0));

    char *llvm_ir = LLVMPrintModuleToString(module);
    if (!llvm_ir)
    {
        fprintf(stderr, "Error: Failed to print LLVM IR.\n");
        exit(EXIT_FAILURE);
    }
    printf("%s\n", llvm_ir);
    LLVMDisposeMessage(llvm_ir);

    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    free_ast(ast);

    return 0;
}
