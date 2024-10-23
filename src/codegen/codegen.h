#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm-c/Core.h>
#include <parser/ast.h>
#include <symbol_table/symbol_table.h>

LLVMValueRef generate_code(Node *node, LLVMModuleRef module, LLVMBuilderRef builder, LLVMValueRef printf_func, LLVMValueRef format_str, SymbolTable *sym_table);

#endif // CODEGEN_H
