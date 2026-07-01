#include "codegen/compiler.h"

#include "runtime/opcode.h"
#include "io/ioutils.h"

#include <string.h>
#include <stdlib.h>

static void compile_expr(struct Compiler *compiler, struct Expr *expr, struct Chunk *chunk);
static void emit_byte(struct Chunk *chunk, Byte byte, struct NodeMetadata metadata);


void compiler_init(struct Compiler *compiler) {
    compiler->local_count = 0;
}

static size_t emit_jump(struct Chunk *chunk, Byte opcode) {
    emit_byte(chunk, opcode, (struct NodeMetadata){0});
    emit_byte(chunk, 0, (struct NodeMetadata){0}); // placeholder
    return chunk->bytecode->size - 1; // position of operand
}

static void patch_jump(struct Chunk *chunk, size_t offset, size_t target) {
    chunk->bytecode->data[offset] = (Byte)(target - offset - 1);
}

static void begin_scope(struct Compiler *compiler) {
    compiler->scope_depth++;
    compiler->scopes[compiler->scope_depth].count = 0;
    compiler->scopes[compiler->scope_depth].start_slot = compiler->local_count;
}

static void end_scope(struct Compiler *compiler) {
    compiler->local_count = compiler->scopes[compiler->scope_depth].start_slot;
    compiler->scope_depth--;
}

static void emit_byte(struct Chunk *chunk, Byte byte, struct NodeMetadata metadata) {
    chunk_write(chunk, byte, metadata.line);
}

static int add_local(struct Compiler *compiler, const char *name) {
    struct Scope *scope = &compiler->scopes[compiler->scope_depth];

    int slot = compiler->local_count++;

    scope->symbols[scope->count++] = (struct Symbol){
        .name = name,
        .slot = slot
    };

    return slot;
}

static int find_local(struct Compiler *compiler, const char *name) {
    for (int i = compiler->scope_depth; i >= 0; i--) {
        struct Scope *scope = &compiler->scopes[i];

        for (size_t j = 0; j < scope->count; j++) {
            if (strcmp(scope->symbols[j].name, name) == 0) {
                return scope->symbols[j].slot;
            }
        }
    }
    return -1; // not found
}

static void compile_literal(Value v, struct Chunk *chunk, struct NodeMetadata metadata) {
    size_t constant_index = chunk_addconstant(chunk, v);
    emit_byte(chunk, OP_LOADCONST, metadata);
    emit_byte(chunk, constant_index, metadata);
}


static void compile_binary(struct Compiler *compiler, struct Expr *expr, struct Chunk *chunk) {
    compile_expr(compiler, expr->value.binop.left, chunk);
    compile_expr(compiler, expr->value.binop.right, chunk);

    switch (expr->value.binop.op) {
        case BIN_ADD:  
            emit_byte(chunk, OP_IADD, expr->metadata); 
            break;
        case BIN_SUB: 
            emit_byte(chunk, OP_ISUB, expr->metadata); 
            break;
        case BIN_MUL:  
            emit_byte(chunk, OP_IMUL, expr->metadata); 
            break;
        case BIN_DIV: 
            emit_byte(chunk, OP_IDIV, expr->metadata); 
            break;
        case BIN_EQ_EQ:
            emit_byte(chunk, OP_ICMP_EQ, expr->metadata);
            break;
        case BIN_BANG_EQ:
            emit_byte(chunk, OP_ICMP_NE, expr->metadata);
            break;
        case BIN_LESSER:
            emit_byte(chunk, OP_ICMP_LT, expr->metadata);
            break;
        case BIN_LESSER_EQ:
            emit_byte(chunk, OP_ICMP_LE, expr->metadata);
            break;
        case BIN_GREATER:
            emit_byte(chunk, OP_ICMP_GT, expr->metadata);
            break;
        case BIN_GREATER_EQ:
            emit_byte(chunk, OP_ICMP_GE, expr->metadata);
            break;
        case BIN_AND:
            emit_byte(chunk, OP_BAND, expr->metadata);
            break;
        case BIN_OR:
            emit_byte(chunk, OP_BOR, expr->metadata);
            break;
    }
}

static void compile_expr(struct Compiler *compiler, struct Expr *expr, struct Chunk *chunk) {
    switch (expr->type) {

        case EX_INT_LITERAL:
            compile_literal((Value){ .int_value = expr->value.intliteral.value}, chunk, expr->metadata);
            break;

        case EX_BINARY:
            compile_binary(compiler, expr, chunk);
            break;
        case EX_VARIABLE: {
            int slot = find_local(compiler, expr->value.variable.name);

            if (slot < 0) {
                eprintf("undefined variable: %s\n", expr->value.variable.name);
                exit(1);
            }

            emit_byte(chunk, OP_ILOADLOCAL, expr->metadata);
            emit_byte(chunk, slot, expr->metadata);
            break;
        }
        case EX_UNARY: {
            compile_expr(compiler, expr->value.unaryop.expr, chunk);

            switch (expr->value.unaryop.op) {
                case UN_NOT:
                    emit_byte(chunk, OP_BNOT, expr->metadata);
                    break;

                case UN_NEGATE:
                    emit_byte(chunk, OP_INEGATE, expr->metadata);
                    break;
            }

            break;
        }
        default:
            // handle error or unreachable
            break;
    }
}


static void compile_stmt(struct Compiler *compiler, struct Stmt *stmt, struct Chunk *chunk) {
    switch (stmt->type) {
        case STMT_PRINT:
            compile_expr(compiler, stmt->value.printstmt.expr, chunk);
            emit_byte(chunk, OP_IPRINT, stmt->metadata);
            break;
        case STMT_STMTEXPR:
            compile_expr(compiler, stmt->value.exprstmt.expr, chunk);
            emit_byte(chunk, OP_POP, stmt->metadata);
            break;
        case STMT_VARDECL: {
            int slot = add_local(compiler, stmt->value.variabledecl.name);
            if (stmt->value.variabledecl.init != NULL) {
                compile_expr(compiler, stmt->value.variabledecl.init, chunk);
            } else {
                compile_literal((Value){ .int_value = 0 }, chunk, stmt->metadata);
            }

            emit_byte(chunk, OP_ISTORELOCAL, stmt->metadata);
            emit_byte(chunk, slot, stmt->metadata);
            break;
        }
        case STMT_VARASSIGN: {
            compile_expr(compiler, stmt->value.variableassignment.value, chunk);
            int slot = find_local(compiler, stmt->value.variableassignment.name);
            if (slot < 0) {
                eprintf("Undeclared variable: %s\n", stmt->value.variableassignment.name);
                exit(1);
            }
            emit_byte(chunk, OP_ISTORELOCAL, stmt->metadata);
            emit_byte(chunk, slot, stmt->metadata);
            break;
        }
        case STMT_IF: {
            compile_expr(compiler, stmt->value.ifstmt.condition, chunk);

            size_t jump_to_else = emit_jump(chunk, OP_JIF);

            compile_stmt(compiler, stmt->value.ifstmt.then_branch, chunk);

            size_t jump_to_end = emit_jump(chunk, OP_JMP);

            size_t else_pos = chunk->bytecode->size;

            if (stmt->value.ifstmt.else_branch) {
                compile_stmt(compiler, stmt->value.ifstmt.else_branch, chunk);
            }

            size_t end_pos = chunk->bytecode->size;

            patch_jump(chunk, jump_to_else, else_pos);
            patch_jump(chunk, jump_to_end, end_pos);

            break;
        }
        case STMT_WHILE: {
            size_t loop_start = chunk->bytecode->size;

            compile_expr(compiler, stmt->value.whilestmt.condition, chunk);

            size_t exit_jump = emit_jump(chunk, OP_JIF);

            compile_stmt(compiler, stmt->value.whilestmt.body, chunk);

            emit_byte(chunk, OP_JMP, stmt->metadata);
            emit_byte(chunk, loop_start, stmt->metadata);

            size_t loop_end = chunk->bytecode->size;

            patch_jump(chunk, exit_jump, loop_end);

            break;
        }
        case STMT_BLOCK: {
            begin_scope(compiler);

            for (size_t i = 0; i < stmt->value.block.count; i++) {
                compile_stmt(compiler, stmt->value.block.stmts[i], chunk);
            }

            end_scope(compiler);
            break;
        }
    }
}


void compiler_compile(struct Compiler *compiler, struct Program *program, struct Chunk *chunk) {
    compiler->scope_depth = -1;
    begin_scope(compiler);
    for (size_t i = 0; i < program->size; i++) {
        compile_stmt(compiler, program->stmts[i], chunk);
    }

    emit_byte(chunk, OP_HALT, (struct NodeMetadata){0});
}

void compiler_free(struct Compiler *compiler) {
    (void)compiler;
}
