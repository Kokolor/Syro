// codegen.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include "codegen.h"

LLVMTypeRef get_llvm_type(const char *type_name)
{
    if (strcmp(type_name, "i32") == 0)
        return LLVMInt32Type();
    else if (strcmp(type_name, "void") == 0)
        return LLVMVoidType();
    else
    {
        fprintf(stderr, "[codegen.c][get_llvm_type]: Unsupported type '%s'.\n", type_name);
        exit(EXIT_FAILURE);
    }
}

LLVMValueRef generate_code(Node *node, LLVMModuleRef module, LLVMValueRef printf_func,
                           LLVMValueRef format_str, SymbolTable *sym_table, LLVMBuilderRef builder)
{
    if (!node)
    {
        fprintf(stderr, "[codegen.c][generate_code]: Error: Null AST node.\n");
        exit(EXIT_FAILURE);
    }

    switch (node->type)
    {
    case AST_FUNCTION_DECL:
    {
        LLVMTypeRef return_type = LLVMVoidType();
        if (node->return_type)
        {
            return_type = get_llvm_type(node->return_type);
        }

        LLVMTypeRef *param_types = malloc(sizeof(LLVMTypeRef) * node->param_count);
        for (int i = 0; i < node->param_count; ++i)
        {
            param_types[i] = get_llvm_type(node->parameters[i]->var_type);
        }

        LLVMTypeRef func_type = LLVMFunctionType(return_type, param_types, node->param_count, 0);
        LLVMValueRef func = LLVMAddFunction(module, node->func_name, func_type);

        LLVMBasicBlockRef func_entry = LLVMAppendBasicBlock(func, "entry");
        LLVMBuilderRef func_builder = LLVMCreateBuilder();
        LLVMPositionBuilderAtEnd(func_builder, func_entry);

        SymbolTable *func_sym_table = create_symbol_table();

        for (int i = 0; i < node->param_count; ++i)
        {
            LLVMValueRef param = LLVMGetParam(func, i);
            char *param_name = node->parameters[i]->var_name;
            LLVMTypeRef param_type = param_types[i];
            LLVMValueRef alloca = LLVMBuildAlloca(func_builder, param_type, param_name);
            LLVMBuildStore(func_builder, param, alloca);
            add_symbol(func_sym_table, param_name, alloca);
        }

        generate_code(node->body, module, printf_func, format_str, func_sym_table, func_builder);

        if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(func_builder)) == NULL)
        {
            if (LLVMGetReturnType(func_type) == LLVMVoidType())
            {
                LLVMBuildRetVoid(func_builder);
            }
            else
            {
                LLVMBuildRet(func_builder, LLVMConstNull(return_type));
            }
        }

        LLVMDisposeBuilder(func_builder);
        free_symbol_table(func_sym_table);
        free(param_types);

        return func;
    }

    case AST_RETURN_STMT:
    {
        if (!builder)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Builder is NULL in return statement.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef expr = NULL;
        if (node->expression)
        {
            expr = generate_code(node->expression, module, printf_func, format_str, sym_table, builder);
            LLVMBuildRet(builder, expr);
        }
        else
        {
            LLVMBuildRetVoid(builder);
        }
        return expr;
    }

    case AST_STATEMENT_LIST:
    {
        Node *current = node;
        while (current != NULL)
        {
            generate_code(current->left, module, printf_func, format_str, sym_table, builder);
            current = current->right;
        }
        break;
    }

    case AST_PRINT:
    {
        if (!builder)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Builder is NULL in print statement.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef expr = generate_code(node->expression, module, printf_func, format_str, sym_table, builder);
        if (!expr)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to generate expression for print.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef format_str_ptr = LLVMBuildBitCast(builder, format_str, LLVMPointerType(LLVMInt8Type(), 0), "fmt_ptr");
        LLVMValueRef args[] = {format_str_ptr, expr};
        LLVMTypeRef printf_func_type = LLVMGetElementType(LLVMTypeOf(printf_func));
        LLVMValueRef printf_call = LLVMBuildCall2(builder, printf_func_type, printf_func, args, 2, "callprintf");
        if (!printf_call)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to build call to printf.\n");
            exit(EXIT_FAILURE);
        }

        return printf_call;
    }

    case AST_VARIABLE_DECL:
    {
        if (!builder)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Builder is NULL in variable declaration.\n");
            exit(EXIT_FAILURE);
        }

        LLVMTypeRef var_type = get_llvm_type(node->var_type);
        if (var_type == LLVMVoidType())
        {
            fprintf(stderr, "[codegen.c][generate_code]: Cannot declare variable of type 'void'.\n");
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

        LLVMValueRef expr = generate_code(node->expression, module, printf_func, format_str, sym_table, builder);
        if (!expr)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to generate expression for variable '%s'.\n", var_name);
            exit(EXIT_FAILURE);
        }

        LLVMBuildStore(builder, expr, alloca);
        return alloca;
    }

    case AST_IDENTIFIER:
    {
        if (!builder)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Builder is NULL in identifier.\n");
            exit(EXIT_FAILURE);
        }

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
        return LLVMConstInt(LLVMInt32Type(), node->number_value, 0);

    case AST_PLUS:
    case AST_MINUS:
    case AST_STAR:
    case AST_SLASH:
    {
        if (!builder)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Builder is NULL in binary operation.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef left = generate_code(node->left, module, printf_func, format_str, sym_table, builder);
        LLVMValueRef right = generate_code(node->right, module, printf_func, format_str, sym_table, builder);
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

    case AST_FUNCTION_CALL:
    {
        if (!builder)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Builder is NULL in function call.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef function = LLVMGetNamedFunction(module, node->func_name);
        if (!function)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Function '%s' not found.\n", node->func_name);
            exit(EXIT_FAILURE);
        }

        LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * node->param_count);
        for (int i = 0; i < node->param_count; ++i)
        {
            args[i] = generate_code(node->parameters[i], module, printf_func, format_str, sym_table, builder);
            if (!args[i])
            {
                fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to generate code for argument %d in function call '%s'.\n", i, node->func_name);
                exit(EXIT_FAILURE);
            }
        }

        LLVMTypeRef function_type = LLVMGetElementType(LLVMTypeOf(function));
        LLVMValueRef call = LLVMBuildCall2(builder, function_type, function, args, node->param_count, "");
        if (!call)
        {
            fprintf(stderr, "[codegen.c][generate_code]: Error: Failed to build function call '%s'.\n", node->func_name);
            exit(EXIT_FAILURE);
        }

        free(args);
        return call;
    }

    default:
        fprintf(stderr, "[codegen.c][generate_code]: Error: Unknown AST node type during code generation.\n");
        exit(EXIT_FAILURE);
    }

    return NULL;
}
