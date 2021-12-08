/*
 * Copyright (c) 2021, Dana Burkart <dana.burkart@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef TOKEN_H
#define TOKEN_H

#include <stdlib.h>
#include <stdint.h>

// Lexeme Enumeration
enum token_type_e
{
    // Delimiters
    TOK_L_PAREN,
    TOK_R_PAREN,
    TOK_L_BRACE,
    TOK_R_BRACE,
    TOK_COLON,
    TOK_COMMA,
    TOK_EOL,
    TOK_WILDCARD_CASE,

    TOK_EQUAL,
    TOK_BANG,

    // Math
    TOK_PLUS,
    TOK_MINUS,
    TOK_ASTERISK,
    TOK_SLASH,
    TOK_MODULO,

    // Logical Conjunctions
    TOK_AND,
    TOK_OR,

    // Comparators
    TOK_GREATER,
    TOK_GREATER_EQUAL,
    TOK_LESS,
    TOK_LESS_EQUAL,
    TOK_EQUAL_EQUAL,
    TOK_BANG_EQUAL,

    // Literals
    TOK_IDENTIFIER,
    TOK_STRING,
    TOK_NUMBER,
    TOK_FLOAT,

    // Keywords
    TOK_VAR,
    TOK_FN,
    TOK_RETURN,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NIL,
    TOK_FOR,
    TOK_IN,
    TOK_LET,
    TOK_EXPORTED,
    TOK_IMPORT,
    TOK_MATCH,
    TOK_CASE,

    // Branching
    TOK_IF,

    // Other
    TOK_R_ARROW,
    TOK_DOT,
    TOK_DOT_DOT,

    // Invalid token
    TOK_INVALID,

    TOK_EOF // 'EOF' is reserved on some platforms
};

// Token specific information
typedef struct
{
    enum token_type_e type;
    // Positional data, relative to the original buffer
    unsigned long start;
    unsigned long end;
} token_t;

// List of tokens to be fed to the parser
typedef struct
{
    size_t size;
    size_t capacity;
    token_t *tokens;
} token_list_t;

token_list_t token_list_create(size_t);
void token_list_destroy(token_list_t);
void token_list_add(token_list_t *, token_t);
void token_list_print(token_list_t);
const char *token_name(token_t);

#endif
