// codegen.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <error.h>
#include "codegen.h"

LLVMTypeRef get_llvm_type(const char *type_name)
{
    if (strchr(type_name, '*') != NULL)
    {
        int ptr_count = 0;
        const char *ptr = type_name;
        while (*ptr == '*')
        {
            ptr_count++;
            ptr++;
        }

        char *base_type_name = strndup(type_name, strlen(type_name) - ptr_count);
        LLVMTypeRef base_type = get_llvm_type(base_type_name);
        free(base_type_name);

        LLVMTypeRef ptr_type = base_type;
        for (int i = 0; i < ptr_count; ++i)
        {
            ptr_type = LLVMPointerType(ptr_type, 0);
        }

        return ptr_type;
    }
    else if (strchr(type_name, '[') != NULL)
    {
        char *element_type_str = strdup(type_name);
        if (!element_type_str)
        {
            error_report(-1, "Memory allocation failed in get_llvm_type.\n");
            exit(EXIT_FAILURE);
        }

        char *bracket_pos = strchr(element_type_str, '[');
        *bracket_pos = '\0';
        char *size_str = bracket_pos + 1;
        char *end_bracket = strchr(size_str, ']');
        if (!end_bracket)
        {
            error_report(-1, "Invalid array type '%s'.\n", type_name);
            free(element_type_str);
            exit(EXIT_FAILURE);
        }
        *end_bracket = '\0';

        int size = atoi(size_str);
        if (size <= 0)
        {
            error_report(-1, "Invalid array size in type '%s'.\n", type_name);
            free(element_type_str);
            exit(EXIT_FAILURE);
        }
        LLVMTypeRef element_type = get_llvm_type(element_type_str);
        free(element_type_str);
        return LLVMArrayType(element_type, size);
    }
    else if (strcmp(type_name, "i8") == 0)
        return LLVMInt8Type();
    else if (strcmp(type_name, "i16") == 0)
        return LLVMInt16Type();
    else if (strcmp(type_name, "i32") == 0)
        return LLVMInt32Type();
    else if (strcmp(type_name, "i64") == 0)
        return LLVMInt64Type();
    else if (strcmp(type_name, "void") == 0)
        return LLVMVoidType();
    else
    {
        error_report(-1, "Unsupported type '%s'.\n", type_name);
        exit(EXIT_FAILURE);
    }
}

LLVMValueRef generate_code(Node *node, LLVMModuleRef module, LLVMValueRef printf_func, LLVMValueRef format_str, SymbolTable *sym_table, LLVMBuilderRef builder)
{
    if (!node)
    {
        fprintf(stderr, "Error: Null AST node.\n");
        exit(EXIT_FAILURE);
    }

    switch (node->type)
    {
    case AST_ASSIGNMENT:
    {
        if (!builder)
        {
            error_report(-1, "Builder is NULL in assignment.\n");
            exit(EXIT_FAILURE);
        }

        char *var_name = node->var_name;
        LLVMValueRef var = get_symbol(sym_table, var_name);
        if (!var)
        {
            error_report(-1, "Undefined variable '%s' in assignment.\n", var_name);
            exit(EXIT_FAILURE);
        }

        LLVMValueRef expr = generate_code(node->expression, module, printf_func, format_str, sym_table, builder);
        LLVMBuildStore(builder, expr, var);

        return expr;
    }
    case AST_ADDRESS_OF:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in address-of operation.\n");
            exit(EXIT_FAILURE);
        }

        if (node->expression->type != AST_IDENTIFIER)
        {
            fprintf(stderr, "Error: Can only take address of a variable.\n");
            exit(EXIT_FAILURE);
        }

        char *var_name = node->expression->var_name;
        LLVMValueRef var = get_symbol(sym_table, var_name);
        if (!var)
        {
            fprintf(stderr, "Error: Undefined variable '%s' in address-of operation.\n", var_name);
            exit(EXIT_FAILURE);
        }

        return var;
    }
    case AST_DEREFERENCE:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in dereference operation.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef expr = generate_code(node->expression, module, printf_func, format_str, sym_table, builder);
        LLVMTypeRef expr_type = LLVMTypeOf(expr);

        if (LLVMGetTypeKind(expr_type) != LLVMPointerTypeKind)
        {
            fprintf(stderr, "Error: Cannot dereference a non-pointer type.\n");
            exit(EXIT_FAILURE);
        }

        LLVMTypeRef pointed_type = LLVMGetElementType(expr_type);
        LLVMValueRef loaded = LLVMBuildLoad2(builder, pointed_type, expr, "deref");

        return loaded;
    }
    case AST_ARRAY_TYPE:
    {
        LLVMTypeRef element_type = get_llvm_type(node->var_type);
        LLVMTypeRef array_type = LLVMArrayType(element_type, node->number_value);
        return array_type;
    }
    case AST_ARRAY_DECL:
    {
        LLVMTypeRef element_type = get_llvm_type(node->var_type);
        LLVMTypeRef array_type = LLVMArrayType(element_type, node->number_value);
        LLVMValueRef alloca = LLVMBuildAlloca(builder, array_type, node->var_name);
        add_symbol(sym_table, node->var_name, alloca);
        for (int i = 0; i < node->param_count; ++i)
        {
            LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
            LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0), index};
            LLVMValueRef element_ptr = LLVMBuildGEP2(builder, array_type, alloca, indices, 2, "arrayelem");
            LLVMValueRef element_value = generate_code(node->parameters[i], module, printf_func, format_str, sym_table, builder);
            LLVMBuildStore(builder, element_value, element_ptr);
        }
        return alloca;
    }
    case AST_ARRAY_ASSIGNMENT:
    {
        LLVMValueRef array_ptr = get_symbol(sym_table, node->var_name);
        if (!array_ptr)
        {
            error_report(-1, "Undefined array '%s'.\n", node->var_name);
            exit(EXIT_FAILURE);
        }

        LLVMValueRef index = generate_code(node->left, module, printf_func, format_str, sym_table, builder);
        LLVMValueRef value = generate_code(node->right, module, printf_func, format_str, sym_table, builder);

        LLVMTypeRef array_ptr_type = LLVMTypeOf(array_ptr);
        LLVMTypeRef array_type = LLVMGetElementType(array_ptr_type);
        LLVMTypeRef element_type = LLVMGetElementType(array_type);

        LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 0);
        LLVMValueRef indices[] = {zero, index};

        LLVMValueRef element_ptr = LLVMBuildGEP2(builder, array_type, array_ptr, indices, 2, "arrayelem");

        LLVMBuildStore(builder, value, element_ptr);
        return value;
    }
    case AST_ARRAY_ACCESS:
    {
        LLVMValueRef array_ptr = get_symbol(sym_table, node->var_name);
        if (!array_ptr)
        {
            fprintf(stderr, "Error: Undefined array '%s'.\n", node->var_name);
            exit(EXIT_FAILURE);
        }
        LLVMValueRef index = generate_code(node->expression, module, printf_func, format_str, sym_table, builder);

        LLVMTypeRef array_ptr_type = LLVMTypeOf(array_ptr);

        LLVMTypeRef array_type = LLVMGetElementType(array_ptr_type);

        LLVMTypeRef element_type = LLVMGetElementType(array_type);

        LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 0);
        LLVMValueRef indices[] = {zero, index};

        LLVMValueRef element_ptr = LLVMBuildGEP2(builder, array_type, array_ptr, indices, 2, "arrayelem");

        LLVMValueRef loaded = LLVMBuildLoad2(builder, element_type, element_ptr, "loadelem");
        return loaded;
    }

    case AST_NEGATE:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in negate operation.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef expr = generate_code(node->left, module, printf_func, format_str, sym_table, builder);
        if (!expr)
        {
            fprintf(stderr, "Error: Failed to generate expression for negate.\n");
            exit(EXIT_FAILURE);
        }

        LLVMTypeRef expr_type = LLVMTypeOf(expr);

        if (LLVMGetTypeKind(expr_type) == LLVMIntegerTypeKind)
        {
            return LLVMBuildNeg(builder, expr, "negtmp");
        }
        else
        {
            fprintf(stderr, "Error: Unsupported type in negate operation.\n");
            exit(EXIT_FAILURE);
        }
    }

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
            fprintf(stderr, "Error: Builder is NULL in return statement.\n");
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
    case AST_IF_STATEMENT:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in if statement.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef condition = generate_code(node->condition, module, printf_func, format_str, sym_table, builder);

        LLVMValueRef zero = LLVMConstInt(LLVMTypeOf(condition), 0, 0);
        LLVMValueRef cond = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "ifcond");

        LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "then");
        LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "else");
        LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "ifcont");

        if (node->else_branch)
        {
            LLVMBuildCondBr(builder, cond, then_block, else_block);
        }
        else
        {
            LLVMBuildCondBr(builder, cond, then_block, merge_block);
        }

        LLVMPositionBuilderAtEnd(builder, then_block);
        generate_code(node->then_branch, module, printf_func, format_str, sym_table, builder);
        if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder)) == NULL)
            LLVMBuildBr(builder, merge_block);

        if (node->else_branch)
        {
            LLVMPositionBuilderAtEnd(builder, else_block);
            generate_code(node->else_branch, module, printf_func, format_str, sym_table, builder);
            if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder)) == NULL)
                LLVMBuildBr(builder, merge_block);
        }
        else
        {
            LLVMDeleteBasicBlock(else_block);
        }

        LLVMPositionBuilderAtEnd(builder, merge_block);

        break;
    }
    case AST_PRINT:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in print statement.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef expr = generate_code(node->expression, module, printf_func, format_str, sym_table, builder);
        if (!expr)
        {
            fprintf(stderr, "Error: Failed to generate expression for print.\n");
            exit(EXIT_FAILURE);
        }

        LLVMTypeRef expr_type = LLVMTypeOf(expr);
        LLVMValueRef format_str_ptr;

        if (LLVMGetTypeKind(expr_type) == LLVMIntegerTypeKind)
        {
            format_str_ptr = LLVMBuildBitCast(builder, format_str, LLVMPointerType(LLVMInt8Type(), 0), "fmt_ptr");
        }
        else if (LLVMGetTypeKind(expr_type) == LLVMPointerTypeKind)
        {
            LLVMValueRef ptr_format_str = LLVMBuildGlobalStringPtr(builder, "%p\n", "ptrfmt");
            format_str_ptr = LLVMBuildBitCast(builder, ptr_format_str, LLVMPointerType(LLVMInt8Type(), 0), "fmt_ptr");
        }
        else
        {
            fprintf(stderr, "Error: Unsupported type in print statement.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef args[] = {format_str_ptr, expr};

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
            fprintf(stderr, "Error: Failed to build call to printf.\n");
            exit(EXIT_FAILURE);
        }

        return printf_call;
    }
    case AST_DEREFERENCE_ASSIGNMENT:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in dereference assignment.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef ptr = generate_code(node->left, module, printf_func, format_str, sym_table, builder);
        if (LLVMGetTypeKind(LLVMTypeOf(ptr)) != LLVMPointerTypeKind)
        {
            fprintf(stderr, "Error: Left side of dereference assignment is not a pointer.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef value = generate_code(node->right, module, printf_func, format_str, sym_table, builder);
        LLVMBuildStore(builder, value, ptr);
        return value;
    }

    case AST_VARIABLE_DECL:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in variable declaration.\n");
            exit(EXIT_FAILURE);
        }

        LLVMTypeRef var_type = get_llvm_type(node->var_type);
        if (var_type == LLVMVoidType())
        {
            fprintf(stderr, "Error: Cannot declare variable of type 'void'.\n");
            exit(EXIT_FAILURE);
        }

        char *var_name = node->var_name;

        LLVMValueRef alloca = LLVMBuildAlloca(builder, var_type, var_name);
        if (!alloca)
        {
            fprintf(stderr, "Error: Failed to allocate memory for variable '%s'.\n", var_name);
            exit(EXIT_FAILURE);
        }

        add_symbol(sym_table, var_name, alloca);

        if (node->expression)
        {
            LLVMValueRef expr = generate_code(node->expression, module, printf_func, format_str, sym_table, builder);
            if (!expr)
            {
                fprintf(stderr, "Error: Failed to generate expression for variable '%s'.\n", var_name);
                exit(EXIT_FAILURE);
            }

            LLVMBuildStore(builder, expr, alloca);
        }
        else
        {
            LLVMBuildStore(builder, LLVMConstNull(var_type), alloca);
        }

        return alloca;
    }
    case AST_IDENTIFIER:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in identifier.\n");
            exit(EXIT_FAILURE);
        }

        char *var_name = node->var_name;
        LLVMValueRef var = get_symbol(sym_table, var_name);
        if (!var)
        {
            fprintf(stderr, "Error: Undefined variable '%s'.\n", var_name);
            exit(EXIT_FAILURE);
        }

        LLVMTypeRef var_ptr_type = LLVMTypeOf(var);
        LLVMTypeRef var_type = LLVMGetElementType(var_ptr_type);

        LLVMValueRef loaded = LLVMBuildLoad2(builder, var_type, var, var_name);
        if (!loaded)
        {
            fprintf(stderr, "Error: Failed to load variable '%s'.\n", var_name);
            exit(EXIT_FAILURE);
        }

        return loaded;
    }
    case AST_NUMBER:
        return LLVMConstInt(LLVMInt32Type(), node->number_value, 0);
    case AST_EQUAL_EQUAL:
    case AST_BANG_EQUAL:
    case AST_LESS:
    case AST_LESS_EQUAL:
    case AST_GREATER:
    case AST_GREATER_EQUAL:
    {
        LLVMValueRef left = generate_code(node->left, module, printf_func, format_str, sym_table, builder);
        LLVMValueRef right = generate_code(node->right, module, printf_func, format_str, sym_table, builder);

        LLVMValueRef cmp_result;

        switch (node->type)
        {
        case AST_EQUAL_EQUAL:
            cmp_result = LLVMBuildICmp(builder, LLVMIntEQ, left, right, "eqtmp");
            break;
        case AST_BANG_EQUAL:
            cmp_result = LLVMBuildICmp(builder, LLVMIntNE, left, right, "netmp");
            break;
        case AST_LESS:
            cmp_result = LLVMBuildICmp(builder, LLVMIntSLT, left, right, "slttmp");
            break;
        case AST_LESS_EQUAL:
            cmp_result = LLVMBuildICmp(builder, LLVMIntSLE, left, right, "sletmp");
            break;
        case AST_GREATER:
            cmp_result = LLVMBuildICmp(builder, LLVMIntSGT, left, right, "sgttmp");
            break;
        case AST_GREATER_EQUAL:
            cmp_result = LLVMBuildICmp(builder, LLVMIntSGE, left, right, "sgetmp");
            break;
        default:
            fprintf(stderr, "Error: Unknown comparison operator.\n");
            exit(EXIT_FAILURE);
        }

        return LLVMBuildZExt(builder, cmp_result, LLVMInt32Type(), "booltmp");
    }
    case AST_PLUS:
    case AST_MINUS:
    case AST_STAR:
    case AST_SLASH:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in binary operation.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef left = generate_code(node->left, module, printf_func, format_str, sym_table, builder);
        LLVMValueRef right = generate_code(node->right, module, printf_func, format_str, sym_table, builder);
        if (!left || !right)
        {
            fprintf(stderr, "Error: Failed to generate operands for binary operation.\n");
            exit(EXIT_FAILURE);
        }

        LLVMTypeRef left_type = LLVMTypeOf(left);
        LLVMTypeRef right_type = LLVMTypeOf(right);

        if (LLVMGetTypeKind(left_type) != LLVMGetTypeKind(right_type))
        {
            fprintf(stderr, "Error: Type mismatch in binary operation.\n");
            exit(EXIT_FAILURE);
        }

        if (LLVMGetTypeKind(left_type) == LLVMIntegerTypeKind)
        {
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
                fprintf(stderr, "Error: Unsupported binary operation.\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            fprintf(stderr, "Error: Unsupported types in binary operation.\n");
            exit(EXIT_FAILURE);
        }
    }
    case AST_FUNCTION_CALL:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in function call.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef function = LLVMGetNamedFunction(module, node->func_name);
        if (!function)
        {
            fprintf(stderr, "Error: Function '%s' not found.\n", node->func_name);
            exit(EXIT_FAILURE);
        }

        LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * node->param_count);
        for (int i = 0; i < node->param_count; ++i)
        {
            args[i] = generate_code(node->parameters[i], module, printf_func, format_str, sym_table, builder);
            if (!args[i])
            {
                fprintf(stderr, "Error: Failed to generate code for argument %d in function call '%s'.\n", i, node->func_name);
                exit(EXIT_FAILURE);
            }
        }

        LLVMTypeRef function_type = LLVMGetElementType(LLVMTypeOf(function));

        LLVMValueRef call = LLVMBuildCall2(builder, function_type, function, args, node->param_count, "");
        if (!call)
        {
            fprintf(stderr, "Error: Failed to build function call '%s'.\n", node->func_name);
            exit(EXIT_FAILURE);
        }

        free(args);
        return call;
    }
    case AST_CAST:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in cast.\n");
            exit(EXIT_FAILURE);
        }

        LLVMValueRef expr = generate_code(node->expression, module, printf_func, format_str, sym_table, builder);
        LLVMTypeRef target_type = get_llvm_type(node->cast_type);

        LLVMTypeRef expr_type = LLVMTypeOf(expr);

        if (LLVMGetTypeKind(expr_type) == LLVMIntegerTypeKind &&
            LLVMGetTypeKind(target_type) == LLVMIntegerTypeKind)
        {
            unsigned src_bits = LLVMGetIntTypeWidth(expr_type);
            unsigned dest_bits = LLVMGetIntTypeWidth(target_type);
            if (src_bits < dest_bits)
            {
                return LLVMBuildSExt(builder, expr, target_type, "sexttmp");
            }
            else if (src_bits > dest_bits)
            {
                return LLVMBuildTrunc(builder, expr, target_type, "trunctmp");
            }
            else
            {
                return LLVMBuildBitCast(builder, expr, target_type, "bitcasttmp");
            }
        }
        else
        {
            fprintf(stderr, "Error: Invalid cast from type to type.\n");
            exit(EXIT_FAILURE);
        }
    }
    case AST_WHILE_STATEMENT:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in while statement.\n");
            exit(EXIT_FAILURE);
        }

        LLVMBasicBlockRef cond_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "whilecond");
        LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "whilebody");
        LLVMBasicBlockRef end_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "whileend");

        LLVMBuildBr(builder, cond_block);

        LLVMPositionBuilderAtEnd(builder, cond_block);
        LLVMValueRef condition = generate_code(node->condition, module, printf_func, format_str, sym_table, builder);
        LLVMValueRef zero = LLVMConstInt(LLVMTypeOf(condition), 0, 0);
        LLVMValueRef cond = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "whilecond");

        LLVMBuildCondBr(builder, cond, body_block, end_block);

        LLVMPositionBuilderAtEnd(builder, body_block);
        generate_code(node->body, module, printf_func, format_str, sym_table, builder);
        LLVMBuildBr(builder, cond_block);

        LLVMPositionBuilderAtEnd(builder, end_block);

        break;
    }
    case AST_FOR_STATEMENT:
    {
        if (!builder)
        {
            fprintf(stderr, "Error: Builder is NULL in for statement.\n");
            exit(EXIT_FAILURE);
        }

        LLVMBasicBlockRef init_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "forinit");
        LLVMBasicBlockRef cond_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "forcond");
        LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "forbody");
        LLVMBasicBlockRef increment_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "forinc");
        LLVMBasicBlockRef end_block = LLVMAppendBasicBlock(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)), "forend");

        LLVMBuildBr(builder, init_block);

        LLVMPositionBuilderAtEnd(builder, init_block);
        if (node->init)
        {
            generate_code(node->init, module, printf_func, format_str, sym_table, builder);
        }
        LLVMBuildBr(builder, cond_block);

        LLVMPositionBuilderAtEnd(builder, cond_block);
        LLVMValueRef condition = NULL;
        if (node->condition)
        {
            condition = generate_code(node->condition, module, printf_func, format_str, sym_table, builder);
        }
        else
        {
            condition = LLVMConstInt(LLVMInt1Type(), 1, 0);
        }
        LLVMValueRef zero = LLVMConstInt(LLVMTypeOf(condition), 0, 0);
        LLVMValueRef cond_value = LLVMBuildICmp(builder, LLVMIntNE, condition, zero, "forcond");
        LLVMBuildCondBr(builder, cond_value, body_block, end_block);

        LLVMPositionBuilderAtEnd(builder, body_block);
        generate_code(node->body, module, printf_func, format_str, sym_table, builder);
        LLVMBuildBr(builder, increment_block);

        LLVMPositionBuilderAtEnd(builder, increment_block);
        if (node->increment)
        {
            generate_code(node->increment, module, printf_func, format_str, sym_table, builder);
        }
        LLVMBuildBr(builder, cond_block);

        LLVMPositionBuilderAtEnd(builder, end_block);

        break;
    }

    default:
        fprintf(stderr, "Error: Unknown AST node type during code generation.\n");
        exit(EXIT_FAILURE);
    }

    return NULL;
}
