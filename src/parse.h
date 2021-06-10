/*
 * Copyright (c) 2021, Dana Burkart <dana.burkart@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef PARSE_H
#define PARSE_H

#include "token.h"
#include "lex.h"

// Data structure representing a node in our abstract syntax tree.
typedef struct expr_t
{
    // TODO: Namespace this enum
    enum
    {
        ASSIGN, BINARY, DECLARE, UNARY, LITERAL, GROUP
    } type;

    union
    {
        struct
        {
            token_t type;
            char *value;
        } literal;

        struct
        {
            char *name;
            struct expr_t* value;
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
            struct expr_t *initial_value;            // NULL if not initialized
        } declare;

        struct
        {
            token_t operator;
            struct expr_t *operand;
        } unary;

        struct expr_t *group;
    } op;
} ast_t;

ast_t *parse(scan_context_t *);

void print_ast(scan_context_t *, ast_t *);

#endif
