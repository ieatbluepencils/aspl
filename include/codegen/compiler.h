#ifndef CODEGEN_COMPILER_H
#define CODEGEN_COMPILER_H

#include "frontend/parser/ast.h"
#include "runtime/chunk.h"

#define MAX_LOCALS 256
#define MAX_SCOPE 64

struct Symbol {
    const char *name;
    Byte slot;
};

struct Scope {
    struct Symbol symbols[MAX_LOCALS];
    size_t count;
    size_t start_slot;
};

struct Patch { 
    size_t position;
    struct Patch *next;
};

struct Compiler {
    struct Scope scopes[MAX_SCOPE];
    int scope_depth;

    int local_count;
};

void compiler_init(struct Compiler *compiler);
void compiler_compile(struct Compiler *compiler, struct Program *program, struct Chunk *chunk);
void compiler_free(struct Compiler *compiler);

#endif // CODEGEN_COMPILER_H
