// codegen.c

#include <stdio.h>
#include <stdlib.h>
#include "codegen.h"

LLVMValueRef generate_code(Node *node, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef printf_func, LLVMValueRef format_str)
{
    if (!node)
    {
        fprintf(stderr, "[codegen.c][generate_code]: Error: Null AST node.\n");
        exit(EXIT_FAILURE);
    }

    switch (node->type)
    {
    case AST_STATEMENT_LIST:
    {
        LLVMValueRef last = NULL;
        Node *current = node;
        while (current != NULL)
        {
            last = generate_code(current->left, module, builder, printf_func, format_str);
            current = current->right;
        }
        return last;
    }

    case AST_PRINT:
    {
        LLVMValueRef expr = generate_code(node->expression, module, builder, printf_func, format_str);
        if (!expr)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to generate expression for print.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef args[] = {format_str, expr};

        LLVMValueRef printf_call = LLVMBuildCall(builder, printf_func, args, 2, "callprintf");
        if (!printf_call)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to build call to printf.\n");
            exit(EXIT_FAILURE);
        }

        return printf_call;
    }

    case AST_NUMBER:
    {
        return LLVMConstInt(LLVMInt32Type(), node->number_value, 0);
    }

    case AST_PLUS:
    case AST_MINUS:
    case AST_STAR:
    case AST_SLASH:
    {
        LLVMValueRef left = generate_code(node->left, module, builder, printf_func, format_str);
        LLVMValueRef right = generate_code(node->right, module, builder, printf_func, format_str);
        if (!left || !right)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to generate operands for binary operation.\n");
            exit(EXIT_FAILURE);
        }

        switch (node->type)
        {
        case AST_PLUS:
            return LLVMBuildAdd(builder, left, right, "addtmp");
        case AST_MINUS:
            return LLVMBuildSub(builder, left, right, "subtmp");
        case AST_STAR:
            return LLVMBuildMul(builder, left, right, "multmp");
        case AST_SLASH:
            return LLVMBuildSDiv(builder, left, right, "divtmp");
        default:
            fprintf(stderr, "[codegen.c][generate_code]: Error: Unsupported binary operation.\n");
            exit(EXIT_FAILURE);
        }
    }

    default:
        fprintf(stderr, "[codegen.c][generate_code]: Error: Unknown AST node type during code generation.\n");
        exit(EXIT_FAILURE);
    }
}
