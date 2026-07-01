#include "frontend/parser/parser.h"

#include "frontend/parser/ast.h"
#include "io/ioutils.h"
#include "io/ioformat.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static struct Expr *parse_expression(struct Parser *parser);
static struct Stmt *parse_stmt(struct Parser *parser);

void parser_init(struct Parser *parser, const char *source, size_t source_len) {
    parser->lexer = NULL;
    if (parser->lexer != NULL) {
        lexer_free(parser->lexer);
        free(parser->lexer);
    }

    parser->lexer = malloc(sizeof(struct Lexer));
    lexer_init(parser->lexer, source, source_len);
    parser->next = lexer_next(parser->lexer);
    parser->lookahead1 = lexer_next(parser->lexer);
    parser->error_count = 0;
}

static struct Token peek(struct Parser *parser) { 
    return parser->next; 
}

static struct Token peek1(struct Parser *parser) {
    return parser->lookahead1;
}

static struct Token advance(struct Parser *parser) {
    struct Token tmp = parser->next;
    parser->next = parser->lookahead1;
    parser->lookahead1 = lexer_next(parser->lexer);
    return tmp;
}

static struct Expr *create_integer_literal(struct Parser *parser, struct Token tok) {
    struct Expr *expr = malloc(sizeof(struct Expr));
    expr->type = EX_INT_LITERAL;
    char buf[32];
    memcpy(buf, parser->lexer->source + tok.offest, tok.length);
    buf[tok.length] = '\0';
    long value = strtol(buf, NULL, 10);

    
    expr->value.intliteral.value = value;

    expr->metadata.line = tok.line;
    expr->metadata.column = tok.column;

    return expr;
}

static void error(struct Parser *parser, struct Token tok, const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "%s%zu:%zu: %s%serror: %s", ANSI_RESET, tok.line + 1, tok.column + 1, ANSI_BRED, ANSI_BOLD, ANSI_RESET);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");


    size_t count = 1;
    size_t n = tok.line;
    while (n >= 10) {
        n /= 10;
        count++;
    }

    for (size_t i = 0; i < count + 1; i++) {
        eprintf(" ");
    }
    eprintf("|\n");

    eprintf("%zu | ", tok.line + 1);

    const char *line_start = parser->lexer->source + tok.offest;
    while (line_start > parser->lexer->source && line_start[-1] != '\n') {
        line_start--;
    }

    const char *line_end = parser->lexer->source + tok.offest;
    while (*line_end && *line_end != '\n') {
        line_end++;
    }

    size_t line_len = line_end - line_start;

    printf("%.*s\n", (int)line_len, line_start);

    for (size_t i = 0; i < count + 1; i++) {
        eprintf(" ");
    }
    eprintf("|");
    for (size_t i = 0; i < tok.column + 1; i++) {
        eprintf(" ");
    }
    eprintf("%s%s^%s\n", ANSI_RESET, ANSI_YELLOW, ANSI_RESET);

    va_end(args);

    parser->error_count++;
}

static void synchronize(struct Parser *parser) {
    while (peek(parser).type != TK_EOF) {
        if (peek(parser).type == TK_SEMICOLON) {
            advance(parser);
            return;
        }

        switch (peek(parser).type) {
            case TK_IF:
            case TK_PRINT:
            case TK_WHILE:
            case TK_BREAK:
            case TK_CONTINUE:
            case TK_LET:
            case TK_ELSE:
                return;
            default: 
                advance(parser);
        }
    }
}

static struct Expr *create_expr_err(struct Token tok, const char* msg) {
    struct Expr *expr = malloc(sizeof(struct Expr));
    expr->type = EX_ERROR;

    expr->metadata.line = tok.line;
    expr->metadata.column = tok.column;

    expr->value.error.msg = msg;

    return expr;
}

static struct Stmt *create_stmt_err(struct Token tok, const char *msg) {
    struct Stmt *stmt = malloc(sizeof(struct Stmt));
    stmt->type = STMT_ERROR;

    stmt->metadata.line = tok.line;
    stmt->metadata.column = tok.column;

    stmt->value.error.msg = msg;

    return stmt;
}

static struct Expr *create_unary_op(struct Expr *operand, enum UnaryOpType op, struct Token tok) {
    struct Expr *expr = malloc(sizeof(struct Expr));
    expr->type = EX_UNARY;
    
    expr->metadata.line = tok.line;
    expr->metadata.column = tok.column;

    expr->value.unaryop.op = op;
    expr->value.unaryop.expr = operand;
    return expr;
}

static struct Expr *parse_primary(struct Parser *parser) {
    if (peek(parser).type == TK_INTEGERLITERAL) {
        struct Token tok = advance(parser);
        return create_integer_literal(parser, tok);
    } else if (peek(parser).type == TK_OPENPAREN) {
        advance(parser);
        struct Expr *expr = parse_expression(parser);
        if (peek(parser).type != TK_CLOSEPAREN) {
            const char *msg = "expected ')'";
            error(parser, peek(parser), msg);
            return create_expr_err(peek(parser), msg);
        }
        advance(parser);
        
        
        return expr;
    } else if (peek(parser).type == TK_IDENTIFIER) {
        struct Token tok = advance(parser);

        struct Expr *expr = malloc(sizeof(struct Expr));
        expr->type = EX_VARIABLE;

        expr->value.variable.name = malloc(tok.length + 1);
        memcpy(expr->value.variable.name,
            parser->lexer->source + tok.offest,
            tok.length);
        expr->value.variable.name[tok.length] = '\0';

        expr->metadata.line = tok.line;
        expr->metadata.column = tok.column;
        return expr;
    }else {
        const char *msg = "unexpected token";
        error(parser, peek(parser), msg);
        return create_expr_err(peek(parser), msg);
    }
}

static struct Expr *create_binary_op(struct Expr *left,
                                     struct Expr *right, enum BinOpType op, struct Token tok) {
    struct Expr *expr = malloc(sizeof(struct Expr));
    expr->type = EX_BINARY;
    expr->value.binop.op = op;
    expr->value.binop.left = left;
    expr->value.binop.right = right;

    expr->metadata.column = tok.column;
    expr->metadata.line = tok.line;

    return expr;
}

static struct Stmt *create_exprstmt(struct Expr *expr, struct Token tok) {
    struct Stmt *stmt = malloc(sizeof(struct Stmt));
    stmt->type = STMT_STMTEXPR;
    stmt->value.exprstmt.expr = expr;
    stmt->metadata.line = tok.line;
    stmt->metadata.column = tok.column;
    return stmt;
}

static struct Stmt *create_printstmt(struct Expr *expr, struct Token tok) {
    struct Stmt *stmt = malloc(sizeof(struct Stmt));
    stmt->type = STMT_PRINT;
    stmt->value.printstmt.expr = expr;
    stmt->metadata.line = tok.line;
    stmt->metadata.column = tok.column;
    return stmt;
}

static struct Expr *parse_unary(struct Parser *parser) {
    if (peek(parser).type == TK_BANG || peek(parser).type == TK_MINUS) {
        struct Token tok = advance(parser);
        struct Expr *right = parse_unary(parser);

        if (tok.type == TK_BANG) {
            return create_unary_op(right, UN_NOT, tok);
        } else {
            return create_unary_op(right, UN_NEGATE, tok);
        }
    }
    return parse_primary(parser);
}

static struct Expr *parse_factor(struct Parser *parser) {
    struct Expr *left = parse_unary(parser);
    while (peek(parser).type == TK_STAR || peek(parser).type == TK_SLASH) {
        struct Token tok = advance(parser);
        enum TokenType type = tok.type;
        struct Expr *right = parse_unary(parser);
        if (type == TK_STAR) {
            left = create_binary_op(left, right, BIN_MUL, tok);
        } else {
            left = create_binary_op(left, right, BIN_DIV, tok);
        }
    }
    return left;
}

static struct Expr *parse_term(struct Parser *parser) {
    struct Expr *left = parse_factor(parser);
    while (peek(parser).type == TK_PLUS || peek(parser).type == TK_MINUS) {
        struct Token tok = advance(parser);
        enum TokenType type = tok.type;
        struct Expr *right = parse_factor(parser);
        if (type == TK_PLUS) {
            left = create_binary_op(left, right, BIN_ADD, tok);
        } else {
            left = create_binary_op(left, right, BIN_SUB, tok);
        } 
    }
    return left;
}

static struct Expr *parse_comparison(struct Parser* parser) {
    struct Expr *left = parse_term(parser);
    while (peek(parser).type == TK_LESS || peek(parser).type == TK_GREATER ||
            peek(parser).type == TK_LESS_EQ || peek(parser).type == TK_GREATER_EQ) {
        struct Token tok = advance(parser);
        struct Expr *right = parse_term(parser);
        if (tok.type == TK_LESS) {
            left = create_binary_op(left, right, BIN_LESSER, tok);
        } else if (tok.type == TK_GREATER) {
            left = create_binary_op(left, right, BIN_GREATER, tok);
        } else if (tok.type == TK_LESS_EQ) {
            left = create_binary_op(left, right, BIN_LESSER_EQ, tok);
        } else {
            left = create_binary_op(left, right, BIN_GREATER_EQ, tok);
        }
    }
    
    return left;
}

static struct Expr *parse_equality(struct Parser *parser) {
    struct Expr *left = parse_comparison(parser);
    while (peek(parser).type == TK_EQUAL_EQUAL || peek(parser).type == TK_BANG_EQ) {
        struct Token tok = advance(parser);
        struct Expr *right = parse_comparison(parser);
        if (tok.type == TK_EQUAL_EQUAL) {
            left = create_binary_op(left, right, BIN_EQ_EQ, tok);
        } else {
            left = create_binary_op(left, right, BIN_BANG_EQ, tok);
        }
    }

    return left;
}

static struct Expr *parse_and(struct Parser *parser) {
    struct Expr *left = parse_equality(parser);
    while (peek(parser).type == TK_AND) {
        struct Token tok = advance(parser);
        struct Expr *right = parse_equality(parser);
        left = create_binary_op(left, right, BIN_AND, tok);
    }

    return left;
}

static struct Expr *parse_or(struct Parser *parser) {
    struct Expr *left = parse_and(parser);
    while (peek(parser).type == TK_OR) {
        struct Token tok = advance(parser);
        struct Expr *right = parse_and(parser);
        left = create_binary_op(left, right, BIN_OR, tok);
    }

    return left;
}

static struct Expr *parse_expression(struct Parser *parser) {
    return parse_or(parser);
}

static bool is_var_type(enum TokenType type) {
    switch (type) {
        case TK_INT:
            return true;
        default:
            return false;
    }
}

static struct Stmt *create_vardecl(const char *name, enum Type type, struct Expr *init, struct Token tok) {
    struct Stmt *stmt = malloc(sizeof(struct Stmt));
    stmt->type = STMT_VARDECL;
    stmt->value.variabledecl.name = name;
    stmt->value.variabledecl.type = type;
    stmt->value.variabledecl.init = init;
    stmt->metadata.line = tok.line;
    stmt->metadata.column = tok.column;
    return stmt;
}

static struct Stmt *parse_vardecl(struct Parser* parser, struct Token start) {
    struct Token tok = advance(parser);

    if (tok.type != TK_IDENTIFIER) {
        const char *msg = "expected identifier";
        error(parser, tok, msg);
        return create_stmt_err(tok, msg);
    }
    char *var_name = malloc(tok.length + 1);
    memcpy(var_name, parser->lexer->source + tok.offest, tok.length);
    var_name[tok.length] = '\0';

    // force type annotation
    if (peek(parser).type != TK_COLON) {
        const char *msg = "expected explicit type annotation";
        error(parser, peek(parser), msg); 
        return create_stmt_err(peek(parser), msg);
    }

    // consume ':'
    advance(parser);

    if (!is_var_type(peek(parser).type)) {
        const char *msg = "expected type after ':'";
        error(parser, peek(parser), msg);
        return create_stmt_err(peek(parser), msg);
    }

    struct Token tok1 = advance(parser);
    enum TokenType type = tok1.type;
    
    enum Type var_type;
    if (type == TK_INT) {
        var_type = TYPE_INT;
    } else {
        const char *msg = "unexpected tokentype";
        error(parser, tok, "%s: %d", msg, type);
        return create_stmt_err(tok, msg);
    }

    if (peek(parser).type == TK_EQUAL) {
        advance(parser);
        struct Expr *expr = parse_expression(parser);
        if (peek(parser).type != TK_SEMICOLON) {
            const char *msg = "expected ';'";
            error(parser, peek(parser), msg);
            return create_stmt_err(peek(parser), msg);
        }
        advance(parser);
        return create_vardecl(var_name, var_type, expr, start);
    } else {
        if (peek(parser).type != TK_SEMICOLON) {
            error(parser, peek(parser), "unexpected token: %d", peek(parser).type);
            exit(1);
        }
        advance(parser);
        return create_vardecl(var_name, var_type, NULL, start);
    }
}

static struct Stmt *create_varassign(char *name, struct Expr *value, struct Token tok) {
    struct Stmt *stmt = malloc(sizeof(struct Stmt));
    stmt->type = STMT_VARASSIGN;
    stmt->metadata.line = tok.line;
    stmt->metadata.column = tok.column;
    stmt->value.variableassignment.name = name;
    stmt->value.variableassignment.value = value;
    return stmt;
}

static struct Stmt *parse_varassign(struct Parser *parser) {
    struct Token name_token = advance(parser);

    if (peek(parser).type != TK_EQUAL) {
        error(parser, peek(parser), "expected '=' after identifier");
        exit(1);
    }

    advance(parser); // consume =


    struct Expr *value = parse_expression(parser);

    if (peek(parser).type != TK_SEMICOLON) {
        error(parser, peek(parser), "expected ';");
        exit(1);
    }

    advance(parser);

    char *name = malloc(name_token.length + 1);
    memcpy(name, parser->lexer->source + name_token.offest, name_token.length);
    name[name_token.length] = '\0';

    return create_varassign(name, value, name_token);
}

static struct Stmt *create_block(struct Stmt **block_stmt, size_t count, struct Token tok) {
    struct Stmt *stmt = malloc(sizeof(struct Stmt));
    stmt->type = STMT_BLOCK;
    stmt->metadata.line = tok.line;
    stmt->metadata.column = tok.column;
    stmt->value.block.stmts = block_stmt;
    stmt->value.block.count = count;
    return stmt;
}

static struct Stmt *parse_block(struct Parser *parser) {
    struct Token tok = advance(parser);
    struct Stmt **stmts = NULL;
    size_t count = 0;
    while (peek(parser).type != TK_CLOSEBRACE && peek(parser).type != TK_EOF) {
        struct Stmt *stmt = parse_stmt(parser);
        stmts = realloc(stmts, sizeof(struct Stmt*) * (count + 1));
        stmts[count++] = stmt;

        
    }

    if (peek(parser).type != TK_CLOSEBRACE) {
        error(parser, peek(parser), "expected '}'");
        exit(1);
    }

    advance(parser);
    return create_block(stmts, count, tok);
}

static struct Stmt *create_ifstmt(struct Expr *cond, struct Stmt *then_branch,
                                  struct Stmt *else_branch,
                                  struct Token tok) {
    struct Stmt *stmt = malloc(sizeof(struct Stmt));
    stmt->type = STMT_IF;
    stmt->metadata.line = tok.line;
    stmt->metadata.column = tok.column;

    stmt->value.ifstmt.condition = cond;
    stmt->value.ifstmt.then_branch = then_branch;
    stmt->value.ifstmt.else_branch = else_branch;

    return stmt;
}

static struct Stmt *create_whilestmt(struct Expr *cond,
                                     struct Stmt *body,
                                     struct Token tok) {
    struct Stmt *stmt = malloc(sizeof(struct Stmt));
    stmt->type = STMT_WHILE;
    stmt->metadata.line = tok.line;
    stmt->metadata.column = tok.column;

    stmt->value.whilestmt.condition = cond;
    stmt->value.whilestmt.body = body;

    return stmt;
}

static struct Stmt *create_breakstmt(struct Token tok) {
    struct Stmt *stmt = malloc(sizeof(struct Stmt));
    stmt->type = STMT_BREAK;
    stmt->metadata.line = tok.line;
    stmt->metadata.column = tok.column;
    return stmt;
}

static struct Stmt *create_continuestmt(struct Token tok) {
    struct Stmt *stmt = malloc(sizeof(struct Stmt));
    stmt->type = STMT_CONTINUE;
    stmt->metadata.line = tok.line;
    stmt->metadata.column = tok.column;
    return stmt;
}

static struct Stmt *parse_if_stmt(struct Parser *parser) {
    struct Token start = advance(parser);
    if (peek(parser).type != TK_OPENPAREN) {
        error(parser, peek(parser), "expected '(' after keyword 'if'");
        exit(1);
    }
    advance(parser);

    struct Expr *condition = parse_expression(parser);

    if (peek(parser).type != TK_CLOSEPAREN) {
        error(parser, peek(parser), "expected ')' after expression");
        exit(1);
    }

    advance(parser);

    struct Stmt *then = NULL;

    if (peek(parser).type == TK_OPENBRACE) {
        then = parse_block(parser);
    } else {
        then = parse_stmt(parser);
    }

    struct Stmt *else_branch = NULL;

    if (peek(parser).type == TK_ELSE) {
        advance(parser);

        else_branch = parse_stmt(parser);
    }

    return create_ifstmt(condition, then, else_branch, start);
}

static struct Stmt *parse_while_stmt(struct Parser *parser) {
    struct Token start = advance(parser);

    if (peek(parser).type != TK_OPENPAREN) {
        error(parser, peek(parser), "expected '(' after while");
        exit(1);
    }

    advance(parser);

    struct Expr *condition = parse_expression(parser);

    if (peek(parser).type != TK_CLOSEPAREN) {
        error(parser, peek(parser), "expected ')'");
        exit(1);
    } 

    advance(parser);

    struct Stmt *body = NULL;

    if (peek(parser).type == TK_OPENBRACE) {
        body = parse_block(parser);
    } else {
        body = parse_stmt(parser);
    }

    return create_whilestmt(condition, body, start);
}

static struct Stmt *parse_break_stmt(struct Parser *parser) {
    struct Token start = advance(parser); 

    if (peek(parser).type != TK_SEMICOLON) {
        error(parser, start, "expected ';'");
        exit(1);
    }
    
    advance(parser);

    return create_breakstmt(start);
}

static struct Stmt *parse_continue_stmt(struct Parser *parser) {
    struct Token start = advance(parser);

    if (peek(parser).type != TK_SEMICOLON) {
        error(parser, start, "expected ';'");
        exit(1);
    }

    advance(parser);

    return create_continuestmt(start);
}

static struct Stmt *parse_stmt(struct Parser *parser) {
    switch (peek(parser).type) {
        case TK_PRINT: {
            struct Token start = advance(parser);
            struct Expr *expr = parse_expression(parser);
            struct Stmt *stmt = create_printstmt(expr, start);
            if (peek(parser).type != TK_SEMICOLON) {
                error(parser, peek(parser), "expected ';'");
                exit(1);
            }
            advance(parser);
            return stmt;
        }    
        case TK_LET: {
            struct Token start = advance(parser);
            struct Stmt *stmt = parse_vardecl(parser, start);
            return stmt;
        }
        case TK_OPENBRACE: {
            return parse_block(parser);
        }
        case TK_IF: {
            return parse_if_stmt(parser);
        }
        case TK_WHILE: {
            return parse_while_stmt(parser);
        }
        case TK_BREAK: {
            return parse_break_stmt(parser);
        }
        case TK_CONTINUE: {
            return parse_continue_stmt(parser);
        }
        case TK_IDENTIFIER: {
            if (peek1(parser).type == TK_EQUAL) {
                return parse_varassign(parser);
            }
            // if not '=' then fall through to expression parsing 
            // because we assume rn that if it isnt x = value; then x is being used in an expression
        }
        case TK_INTEGERLITERAL:
        case TK_OPENPAREN: {
            struct Token start = peek(parser);
            struct Expr *expr = parse_expression(parser);
            struct Stmt *stmt = create_exprstmt(expr, start);
            if (peek(parser).type != TK_SEMICOLON) {
                error(parser, peek(parser), "expected ';'");
                exit(1);
            }
            advance(parser);
            return stmt;
        }
        default:
            error(parser, peek(parser), "unexpected token at statement start");
            exit(1);
    }
}

struct Program *parser_parse(struct Parser *parser) {
    struct Program *program = malloc(sizeof(struct Program));
    program->stmts = NULL;
    program->size = 0;

    while (peek(parser).type != TK_EOF) {
        struct Stmt *stmt = parse_stmt(parser);

        struct Stmt **tmp = realloc(program->stmts, sizeof(struct Stmt*) * (program->size + 1));
        if (!tmp) {
            eprintf("memory allocation failure\n");            
            exit(1); 
        }

        program->stmts = tmp;   
        program->stmts[program->size++] = stmt;
    }


    return program;
}

void parser_free(struct Parser *parser) {
    lexer_free(parser->lexer);
    free(parser->lexer);
    parser->lexer = NULL;
}
