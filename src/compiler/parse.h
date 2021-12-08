/*
 * Copyright (c) 2021, Dana Burkart <dana.burkart@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>
#include <stdint.h>

#include "util/location.h"
#include "token.h"
#include "lex.h"

// Data structure representing a node in our abstract syntax tree.
typedef struct expr_t
{
    enum
    {
        AST_ASSIGN,
        AST_BINARY,
        AST_DECLARE,
        AST_UNARY,
        AST_LITERAL,
        AST_GROUP,
        AST_STMT_LIST,
        AST_CASE_LIST,
        AST_FUNCTION_DECL,
        AST_FUNCTION_CALL,
        AST_EXPR_LIST,
        AST_VAR_LIST,
        AST_MODULE,
        AST_IF_STMT,
        AST_FOR_STMT,
        AST_TUPLE,
        AST_RANGE,
        AST_MATCH_STMT,
        AST_CASE_STMT
    } type;

    union
    {
        struct
        {
            token_t token;
            char *value;
        } literal;

        struct
        {
            char *name;
            struct expr_t *value;
        } assign;

        struct
        {
            token_t operator;
            struct expr_t *left;
            struct expr_t *right;
        } binary;

        struct
        {
            token_t var_type;
            char *name;
            struct expr_t *initial_value; // NULL if not initialized
        } declare;

        struct
        {
            token_t operator;
            struct expr_t *operand;
        } unary;

        struct
        {
            size_t size;
            size_t capacity;
            struct expr_t **items;
        } list;

        struct
        {
            char *name;
            bool exported;
            struct expr_t *args;
            struct expr_t *body;
        } fn;

        struct
        {
            char *name;
            struct expr_t *args;
        } call;

        struct
        {
            struct expr_t *condition;
            struct expr_t *body;
        } if_stmt;

        struct
        {
            char *var;
            struct expr_t *iterable;
            struct expr_t *body;
        } for_stmt;

        struct
        {
            char *var;
            struct expr_t *match_op;
            struct expr_t *body;
        } match_stmt;

        struct
        {
            struct expr_t *condition;
            struct expr_t *body;
        } case_stmt;

        struct
        {
            struct expr_t *begin;
            struct expr_t *end;
        } range;

        struct
        {
            char *name;
        } module;

        struct expr_t *group;
    } op;

    location_t location;
} ast_t;

ast_t *parse(scan_context_t *);

void print_ast(scan_context_t *, ast_t *);

#endif
