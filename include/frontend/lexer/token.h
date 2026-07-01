#ifndef FRONTEND_LEXER_TOKEN_H
#define FRONTEND_LEXER_TOKEN_H

#include <stddef.h>

enum TokenType {
    TK_INTEGERLITERAL,

    TK_OPENPAREN,
    TK_CLOSEPAREN,

    TK_OPENBRACE,
    TK_CLOSEBRACE,

    TK_PLUS,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,

    TK_PRINT,

    TK_SEMICOLON,
    TK_COLON,

    TK_EQUAL,

    TK_EQUAL_EQUAL,
    TK_GREATER,
    TK_LESS,
    TK_GREATER_EQ,
    TK_LESS_EQ,
    TK_OR,
    TK_AND,
    TK_BANG,
    TK_BANG_EQ,

    TK_IDENTIFIER,

    TK_INT,

    TK_IF,
    TK_ELSE,

    TK_WHILE,
    TK_BREAK,
    TK_CONTINUE,

    TK_LET,

    TK_EOF
};

struct Token {
    enum TokenType type;
    size_t offest;
    size_t length;
    size_t line;
    size_t column;
};

#endif // FRONTEND_LEXER_TOKEN_H
