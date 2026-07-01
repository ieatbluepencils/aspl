#include "frontend/parser/ast.h"

#include <stdlib.h>
#include <stddef.h>

static void free_expr(struct Expr *expr);
static void free_stmt(struct Stmt *stmt);

static void free_expr(struct Expr *expr) {
    if (!expr) return;

    switch (expr->type) {
        case EX_INT_LITERAL:
            break;

        case EX_BINARY:
            free_expr(expr->value.binop.left);
            free_expr(expr->value.binop.right);
            break;

        case EX_UNARY:
            free_expr(expr->value.unaryop.expr);
            break;

        case EX_VARIABLE:
            free(expr->value.variable.name);
            break;

        case EX_ERROR:
            free(expr->value.error.msg);

        default:
            break;
    }

    free(expr);
}

static void free_stmt(struct Stmt *stmt) {
    if (!stmt) return;

    switch (stmt->type) {

        case STMT_STMTEXPR:
            free_expr(stmt->value.exprstmt.expr);
            break;

        case STMT_PRINT:
            free_expr(stmt->value.printstmt.expr);
            break;

        case STMT_VARDECL:
            free(stmt->value.variabledecl.name);
            free_expr(stmt->value.variabledecl.init);
            break;

        case STMT_VARASSIGN:
            free(stmt->value.variableassignment.name);
            free_expr(stmt->value.variableassignment.value);
            break;

        case STMT_BLOCK:
            for (size_t i = 0; i < stmt->value.block.count; i++) {
                free_stmt(stmt->value.block.stmts[i]);
            }
            free(stmt->value.block.stmts);
            break;

        case STMT_IF:
            free_expr(stmt->value.ifstmt.condition);
            free_stmt(stmt->value.ifstmt.then_branch);
            free_stmt(stmt->value.ifstmt.else_branch);
            break;

        case STMT_WHILE:
            free_expr(stmt->value.whilestmt.condition);
            free_stmt(stmt->value.whilestmt.body);
            break;

        case STMT_BREAK:
            break;

        case STMT_CONTINUE:
            break;

        case STMT_ERROR: 
            free(stmt->value.error.msg);
            break;            

        default:
            break;
    }

    free(stmt);
}

void free_ast(struct Program *program) {
    if (!program) return;

    for (size_t i = 0; i < program->size; ++i) {
        free_stmt(program->stmts[i]);
    }

    free(program->stmts);
    free(program);
}