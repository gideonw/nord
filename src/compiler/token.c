/*
 * Copyright (c) 2021, Dana Burkart <dana.burkart@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>

#include "token.h"

token_list_t token_list_create(size_t capacity)
{
    token_list_t list;

    list.size = 0;
    list.capacity = capacity;
    if (capacity)
    {
        list.tokens = calloc(capacity, sizeof(token_t));
    }
    else
    {
        list.tokens = NULL;
    }

    return list;
}

void token_list_destroy(token_list_t list)
{
    if (list.tokens)
    {
        free(list.tokens);
    }
}

void token_list_add(token_list_t *list, token_t t)
{
    if (list->size == list->capacity)
    {
        // Grow our capacity by 2x
        list->capacity = list->capacity * 2;
        list->tokens = realloc(list->tokens, sizeof(token_t) * list->capacity);
    }

    list->tokens[list->size] = t;
    list->size = list->size + 1;
}

const char *token_name(token_t t)
{
    switch (t.type)
    {
    // Single character tokens
    case TOK_EQUAL:
        return "EQUAL";
    case TOK_L_PAREN:
        return "L_PAREN";
    case TOK_R_PAREN:
        return "R_PAREN";
    case TOK_L_BRACE:
        return "L_BRACE";
    case TOK_R_BRACE:
        return "R_BRACE";
    case TOK_COLON:
        return "COLON";
    case TOK_COMMA:
        return "COMMA";
    case TOK_EOL:
        return "EOL";
    case TOK_WILDCARD_CASE:
        return "WILDCARD_CASE";

    // BRANCHING
    case TOK_IF:
        return "IF";

    // CONJUNCTIONS
    case TOK_AND:
        return "AND";
    case TOK_OR:
        return "OR";

    // COMPARATORS
    case TOK_BANG:
        return "BANG";
    case TOK_BANG_EQUAL:
        return "BANG_EQUAL";
    case TOK_EQUAL_EQUAL:
        return "EQUAL_EQUAL";
    case TOK_LESS:
        return "LESS";
    case TOK_LESS_EQUAL:
        return "LESS_EQUAL";
    case TOK_GREATER:
        return "GREATER";
    case TOK_GREATER_EQUAL:
        return "GREATER_EQUAL";

    // MATH
    case TOK_MINUS:
        return "MINUS";
    case TOK_PLUS:
        return "PLUS";
    case TOK_ASTERISK:
        return "ASTERISK";
    case TOK_SLASH:
        return "SLASH";
    case TOK_MODULO:
        return "MODULO";

    // Literals
    case TOK_IDENTIFIER:
        return "IDENTIFIER";
    case TOK_STRING:
        return "STRING";
    case TOK_NUMBER:
        return "NUMBER";
    case TOK_FLOAT:
        return "FLOAT";

    // Keywords
    case TOK_TRUE:
        return "TRUE";
    case TOK_FALSE:
        return "FALSE";
    case TOK_NIL:
        return "NIL";
    case TOK_VAR:
        return "VAR";
    case TOK_FN:
        return "FN";
    case TOK_FOR:
        return "FOR";
    case TOK_IN:
        return "IN";
    case TOK_RETURN:
        return "RETURN";
    case TOK_INVALID:
        return "INVALID";
    case TOK_LET:
        return "LET";
    case TOK_EXPORTED:
        return "EXPORTED";
    case TOK_IMPORT:
        return "IMPORT";
    case TOK_MATCH:
        return "MATCH";
    case TOK_CASE:
        return "CASE";

    // Other
    case TOK_DOT:
        return "DOT";
    case TOK_DOT_DOT:
        return "DOT_DOT";
    case TOK_R_ARROW:
        return "R_ARROW";

    // EOF
    case TOK_EOF:
        return "EOF";
    default:
        break;
    }
    // This shouldn't be reachable
    return "UNKNOWN";
}

void token_list_print(token_list_t list)
{
    for (int i = 0; i < list.size; i++)
    {
        token_t t = list.tokens[i];
        printf("[%s:%lu-%lu]\n", token_name(t), t.start, t.end);
    }

    printf("\n");
}
