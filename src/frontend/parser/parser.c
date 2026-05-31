#include "parser.h"
#include "ast.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Expr *parse_expression(struct Parser *parser);

void parser_init(struct Parser *parser, const char *source, size_t source_len) {
    struct Lexer *lexer = malloc(sizeof(struct Lexer));
    lexer_init(lexer, source, source_len);
    parser->lexer = lexer;
    parser->next = lexer_next(parser->lexer);
}

static struct Token peek(struct Parser *parser) { return parser->next; }

static struct Token advance(struct Parser *parser) {
    struct Token tmp = parser->next;
    parser->next = lexer_next(parser->lexer);
    return tmp;
}

static struct Expr *create_integer_literal(struct Parser *parser, struct Token tok) {
    struct Expr *expr = malloc(sizeof(struct Expr));
    expr->type = EX_INT_LITERAL;
    char *strnum = malloc(tok.length + 1);
    strncpy(strnum, parser->lexer->source + tok.offest,
            tok.length);
    strnum[tok.length] = '\0';

    assert(tok.length == strlen(strnum));

    char *end;
    long value = strtol(strnum, &end, 10);

    if (*end != '\0') {
        printf("parsed value: %ld\n", value);
        fprintf(stderr, "parsing failed\n");
        exit(1);
    }

    expr->value.intliteral.value = value;

    free(strnum);

    return expr;
}

static void error(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);

    va_end(args);
}

static struct Expr *parse_primary(struct Parser *parser) {
    if (peek(parser).type == TK_INTEGERLITERAL) {
        struct Token tok = advance(parser);
        return create_integer_literal(parser, tok);
    } else if (peek(parser).type == TK_OPENPAREN) {
        advance(parser);
        struct Expr *expr = parse_expression(parser);
        if (peek(parser).type != TK_CLOSEPAREN) {
            error("expected ')' at %zu:%zu\n", peek(parser).column,
                  peek(parser).line);
            exit(1);
        }
        advance(parser);
        
        return expr;
    } else {
        error("unexpected token\n");
        exit(1);
    }
    exit(1);
}

static struct Expr *create_binary_op(struct Parser *parser, struct Expr *left,
                                     struct Expr *right, enum BinOpType op) {
    struct Expr *expr = malloc(sizeof(struct Expr));
    expr->type = EX_BINARY;
    expr->value.binop.op = op;
    expr->value.binop.left = left;
    expr->value.binop.right = right;

    return expr;
}

static struct Expr *parse_factor(struct Parser *parser) {
    struct Expr *left = parse_primary(parser);
    while (peek(parser).type == TK_STAR || peek(parser).type == TK_SLASH) {
        enum TokenType type = advance(parser).type;
        struct Expr *right = parse_primary(parser);
        if (type == TK_STAR) {
            left = create_binary_op(parser, left, right, BIN_MUL);
        } else {
            left = create_binary_op(parser, left, right, BIN_DIV);
        }
    }
    return left;
}

static struct Expr *parse_term(struct Parser *parser) {
    struct Expr *left = parse_factor(parser);
    while (peek(parser).type == TK_PLUS || peek(parser).type == TK_MINUS) {
        enum TokenType type = advance(parser).type;
        struct Expr *right = parse_factor(parser);
        if (type == TK_PLUS) {
            left = create_binary_op(parser, left, right, BIN_ADD);
        } else {
            left = create_binary_op(parser, left, right, BIN_SUB);
        }
    }
    return left;
}

static struct Expr *parse_expression(struct Parser *parser) {
    return parse_term(parser);
}

struct Expr *parser_parse(struct Parser *parser) {
    struct Expr *expr = parse_expression(parser);
    if (peek(parser).type != TK_EOF) {
        error("unexpected trailing tokens\n");
        exit(1);
    }
    return expr;
}

void parser_free(struct Parser *parser) {
    lexer_free(parser->lexer);
    free(parser->lexer);
    parser->lexer = NULL;
}
