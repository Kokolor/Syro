// codegen.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

LLVMValueRef generate_code(Node *node, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef printf_func, LLVMValueRef format_str, SymbolTable *sym_table)
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
            last = generate_code(current->left, module, builder, printf_func, format_str, sym_table);
            current = current->right;
        }
        return last;
    }

    case AST_PRINT:
    {
        LLVMValueRef expr = generate_code(node->expression, module, builder, printf_func, format_str, sym_table);
        if (!expr)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to generate expression for print.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef args[] = {format_str, expr};

        LLVMTypeRef printf_func_type = LLVMGetElementType(LLVMTypeOf(printf_func));
        LLVMValueRef printf_call = LLVMBuildCall2(
            builder,
            printf_func_type,
            printf_func,
            args,
            2,
            "callprintf");
        if (!printf_call)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to build call to printf.\n");
            exit(EXIT_FAILURE);
        }

        return printf_call;
    }

    case AST_VARIABLE_DECL:
    {
        LLVMTypeRef var_type;

        if (strcmp(node->var_type, "i32") == 0)
            var_type = LLVMInt32Type();
        else
        {
            fprintf(stderr, "[codegen.c][generate_code]: Unsupported variable type '%s'.\n", node->var_type);
            exit(EXIT_FAILURE);
        }

        char *var_name = node->var_name;

        LLVMValueRef alloca = LLVMBuildAlloca(builder, var_type, var_name);
        if (!alloca)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Failed to allocate memory for variable '%s'.\n", var_name);
            exit(EXIT_FAILURE);
        }

        add_symbol(sym_table, var_name, alloca);

        LLVMValueRef expr = generate_code(node->expression, module, builder, printf_func, format_str, sym_table);
        if (!expr)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to generate expression for variable '%s'.\n", var_name);
            exit(EXIT_FAILURE);
        }

        LLVMTypeRef expr_type = LLVMTypeOf(expr);

        LLVMBuildStore(builder, expr, alloca);

        return alloca;
    }

    case AST_IDENTIFIER:
    {
        char *var_name = node->var_name;
        LLVMValueRef var = get_symbol(sym_table, var_name);
        if (!var)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Undefined variable '%s'.\n", var_name);
            exit(EXIT_FAILURE);
        }

        LLVMTypeRef var_ptr_type = LLVMTypeOf(var);
        LLVMTypeRef var_type = LLVMGetElementType(var_ptr_type);

        LLVMValueRef loaded = LLVMBuildLoad2(builder, var_type, var, var_name);
        if (!loaded)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Failed to load variable '%s'.\n", var_name);
            exit(EXIT_FAILURE);
        }

        return loaded;
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
        LLVMValueRef left = generate_code(node->left, module, builder, printf_func, format_str, sym_table);
        LLVMValueRef right = generate_code(node->right, module, builder, printf_func, format_str, sym_table);
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
