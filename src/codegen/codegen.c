// codegen.c

#include <stdio.h>
#include <stdlib.h>
#include "codegen.h"

LLVMValueRef generate_code(Node *node, LLVMModuleRef module, LLVMBuilderRef builder)
{
    if (!node)
        return NULL;

    switch (node->operation)
    {
    case AST_NUMBER:
    {
        LLVMValueRef alloc = LLVMBuildAlloca(builder, LLVMInt32Type(), "num");
        LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), node->number_value, 0), alloc);
        return alloc;
    }

    case AST_PLUS:
    case AST_MINUS:
    case AST_STAR:
    case AST_SLASH:
    {
        LLVMValueRef left_alloc = generate_code(node->left, module, builder);
        LLVMValueRef right_alloc = generate_code(node->right, module, builder);

        LLVMValueRef left_val = LLVMBuildLoad2(builder, LLVMInt32Type(), left_alloc, "left");
        LLVMValueRef right_val = LLVMBuildLoad2(builder, LLVMInt32Type(), right_alloc, "right");

        LLVMValueRef result;
        switch (node->operation)
        {
        case AST_PLUS:
            result = LLVMBuildAdd(builder, left_val, right_val, "addtmp");
            break;
        case AST_MINUS:
            result = LLVMBuildSub(builder, left_val, right_val, "subtmp");
            break;
        case AST_STAR:
            result = LLVMBuildMul(builder, left_val, right_val, "multmp");
            break;
        case AST_SLASH:
            result = LLVMBuildSDiv(builder, left_val, right_val, "divtmp");
            break;
        default:
            fprintf(stderr, "[ast.c][generate_code]: Unknown AST operation during code generation.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef result_alloc = LLVMBuildAlloca(builder, LLVMInt32Type(), "result");

        LLVMBuildStore(builder, result, result_alloc);
        return result_alloc;
    }

    default:
        fprintf(stderr, "[ast.c][generate_code]: Unknown AST operation during code generation.\n");
        exit(EXIT_FAILURE);
    }
}