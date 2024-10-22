// main.c

#include <stdio.h>
#include <stdlib.h>
#include "codegen/codegen.h"
#include "lexer/lexer.h"
#include "parser/ast.h"

int main()
{
    char source[] = "4 * 7 - 3 * 4 - 24 + 57";
    Lexer lexer;
    init_lexer(&lexer, source);

    Node *ast = parse_binary_expression(&lexer);

    LLVMModuleRef module = LLVMModuleCreateWithName("module");
    LLVMBuilderRef builder = LLVMCreateBuilder();

    LLVMTypeRef main_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(module, "main", main_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef ret_alloc = generate_code(ast, module, builder);

    LLVMValueRef ret_val = LLVMBuildLoad2(builder, LLVMInt32Type(), ret_alloc, "ret_val");
    LLVMBuildRet(builder, ret_val);

    char *llvm_ir = LLVMPrintModuleToString(module);
    printf("=== Generated IR ===\n%s\n", llvm_ir);
    LLVMDisposeMessage(llvm_ir);

    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);

    free_ast(ast);

    return 0;
}