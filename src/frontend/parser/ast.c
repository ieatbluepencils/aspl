#include "ast.h"

#include <stdlib.h>
#include <stddef.h>

void free_ast(struct Expr *ast) {
    if (ast == NULL) {
        return;
    }
    switch (ast->type) {
        case EX_BINARY:
            free_ast(ast->value.binop.left);
            free_ast(ast->value.binop.right);
            break;

        case EX_INT_LITERAL:
            break;
    }

    free(ast);
}