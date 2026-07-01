#include "frontend/lexer/lexer.h"

#include "frontend/lexer/token.h"
#include "str/str_intern.h"
#include "io/ioutils.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *KW_print;
const char *KW_let;
const char *KW_int;
const char *KW_if;
const char *KW_else;
const char *KW_while;
const char *KW_continue;
const char *KW_break;


void lexer_init(struct Lexer *lexer, const char *source, size_t source_len) {
    lexer->source = source;
    lexer->current = 0;
    lexer->start = lexer->current;
    lexer->column = 0;
    lexer->start_column = 0;
    lexer->line = 0;
    lexer->source_len = source_len;

    // internalize keywords on init
    #ifndef HAS_INTERNALIZED_KEYWORDS
        KW_print = str_intern("print");
        KW_let = str_intern("let");
        KW_int = str_intern("int");
        KW_if = str_intern("if");
        KW_else = str_intern("else");
        KW_while = str_intern("while");
        KW_continue = str_intern("continue");
        KW_break = str_intern("break");
        #define HAS_INTERNALIZED_KEYWORDS 
    #endif  
}

#define EOF_CHAR '\0'

bool is_at_end(struct Lexer *lexer) {
    if (lexer->current < lexer->source_len) {
        return false;
    }
    return true;
}

static char peek(struct Lexer *lexer) {
    if (is_at_end(lexer)) {
        return EOF_CHAR;
    }
    return lexer->source[lexer->current];
}

static char peekn(struct Lexer *lexer, size_t n) {
    if (lexer->current + n >= lexer->source_len) {
        return EOF_CHAR;
    }
    return lexer->source[lexer->current + n];
}

static char advance(struct Lexer *lexer) {
    if (peek(lexer) == EOF_CHAR) {
        return EOF_CHAR;
    }
    char c = peek(lexer);
    if (c == '\n') {
        lexer->column = 0;
        lexer->line++;
    } else {
        lexer->column++;
    }
    lexer->current++;
    return c;
}

static struct Token gen_token(struct Lexer *lexer, enum TokenType type) {
    return (struct Token){type, lexer->start, lexer->current - lexer->start,
                          lexer->line, lexer->start_column};
}

static bool skip_whitespace(struct Lexer *lexer) {
    bool modified = false;
    while (!is_at_end(lexer) && isspace((unsigned char)peek(lexer))) {
        modified = true;
        advance(lexer);
    }
    return modified;
}

// TODO: Add support for block comments i.e. /* */
static bool skip_comment(struct Lexer *lexer) {
    bool modified = false;
    if (peek(lexer) == '/' && peekn(lexer, 1) == '/') {
        advance(lexer);
        advance(lexer);
        modified = true;
        while (!is_at_end(lexer) && peek(lexer) != '\n') {
            advance(lexer);
        }
    }
    return modified;
}

struct Token lexer_next(struct Lexer *lexer) {
    if (is_at_end(lexer)) {
        return (struct Token){TK_EOF, lexer->current, 0, lexer->line,
                              lexer->column};
    }

    while (true) {
        if (skip_comment(lexer)) {
            continue;
        } else if (skip_whitespace(lexer)) {
            continue;
        } else {
            break;
        }
    }

    if (is_at_end(lexer)) {
        return (struct Token){TK_EOF, lexer->current, 0, lexer->line, lexer->column};
    }

    lexer->start = lexer->current;
    lexer->start_column = lexer->column;


    char c = peek(lexer);

    switch (c) {
    case '(':
        advance(lexer);
        return gen_token(lexer, TK_OPENPAREN);
    case ')':
        advance(lexer);
        return gen_token(lexer, TK_CLOSEPAREN);
    case '+':
        advance(lexer);
        return gen_token(lexer, TK_PLUS);
    case '-':
        advance(lexer);
        return gen_token(lexer, TK_MINUS);
    case '*':
        advance(lexer);
        return gen_token(lexer, TK_STAR);
    case '/':
        advance(lexer);
        return gen_token(lexer, TK_SLASH);
    case ';':
        advance(lexer);
        return gen_token(lexer, TK_SEMICOLON);
    case ':':
        advance(lexer);
        return gen_token(lexer, TK_COLON);
    case '=':
        if (peekn(lexer, 1) == '=') {
            advance(lexer);
            advance(lexer);
            return gen_token(lexer, TK_EQUAL_EQUAL);
        }
        advance(lexer);
        return gen_token(lexer, TK_EQUAL);
    case '{':
        advance(lexer);
        return gen_token(lexer, TK_OPENBRACE);
    case '}':
        advance(lexer);
        return gen_token(lexer, TK_CLOSEBRACE);
    case '>':
        if (peekn(lexer, 1) == '=') {
            advance(lexer);
            advance(lexer);
            return gen_token(lexer, TK_GREATER_EQ);
        }
        advance(lexer);
        return gen_token(lexer, TK_GREATER);
    case '<':
        if (peekn(lexer, 1) == '=') {
            advance(lexer);
            advance(lexer);
            return gen_token(lexer, TK_LESS_EQ);
        }
        advance(lexer);
        return gen_token(lexer, TK_LESS);
    case '|':
        if (peekn(lexer, 1) == '|') {
            advance(lexer);
            advance(lexer);
            return gen_token(lexer, TK_OR);
        }
        eprintf("expected '|' but found: '%c'\n", c);
    case '&':
        if (peekn(lexer, 1) == '&') {
            advance(lexer);
            advance(lexer);
            return gen_token(lexer, TK_AND);
        }
        eprintf("expected '&' but found: '%c'\n", c);
    case '!':
        if (peekn(lexer, 1) == '=') {
            advance(lexer);
            advance(lexer);
            return gen_token(lexer, TK_BANG_EQ);
        }
        return gen_token(lexer, TK_BANG);
    default:
        if (isdigit((unsigned char)c)) {
            advance(lexer);
            while (isdigit((unsigned char)peek(lexer))) {
                advance(lexer);
            }
            return gen_token(lexer, TK_INTEGERLITERAL);
        } else if (isalpha((unsigned char)c) || c == '_') {
            advance(lexer);
            while (isalnum((unsigned char)peek(lexer)) || peek(lexer) == '_') {
                advance(lexer);
            }
            size_t length = lexer->current - lexer->start;
            char *slice = malloc(length + 1);
            strncpy(slice, lexer->source + lexer->start, length);
            slice[length] = '\0';
            const char *s = str_intern(slice);
            free(slice);
            if (s == KW_print) {
                return gen_token(lexer, TK_PRINT);
            } else if (s == KW_int) {
                return gen_token(lexer, TK_INT);
            } else if (s == KW_let) {
                return gen_token(lexer, TK_LET);
            } else if (s == KW_break) {
                return gen_token(lexer, TK_BREAK);
            } else if (s == KW_continue) {
                return gen_token(lexer, TK_CONTINUE);
            } else if (s == KW_else) {
                return gen_token(lexer, TK_ELSE);
            } else if (s == KW_if) {
                return gen_token(lexer, TK_IF);
            } else if (s == KW_while) {
                return gen_token(lexer, TK_WHILE);
            } else {
                return gen_token(lexer, TK_IDENTIFIER);
            }
        } else {
            fprintf(stderr, "Unknown character: '%c'\n", c);
            exit(1);
        }
    }
}

void lexer_free(struct Lexer *lexer) {
    lexer->source = NULL;
    lexer->current = 0;
    lexer->column = 0;
    lexer->line = 0;
    lexer->start = 0;
    lexer->start_column = 0;
}
