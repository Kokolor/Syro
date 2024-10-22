// codegen.h

#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm-c/Core.h>
#include <parser/ast.h>

LLVMValueRef generate_code(Node *node, LLVMModuleRef module, LLVMBuilderRef builder);

#endif // CODEGEN_H