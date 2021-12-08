/*
 * Copyright (c) 2021, Dana Burkart <dana.burkart@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"

/*
 * Return whether or not the specified character is reserved and shouldn't be
 * allowed in an identifier.
 */
bool is_reserved(char c)
{
    switch (c)
    {
    case '(':
    case ')':
    case '{':
    case '}':
    case ':':
    case ',':
    case '*':
    case '+':
    case '/':
    case '-':
    case '=':
        return true;
    }
    return false;
}

/*
 * Return whether or not the specified character is whitespace
 */
bool is_whitespace(char c)
{
    if (c == ' ' || c == '\t' || c == '\n')
        return true;
    return false;
}

bool is_boundary(char c)
{
    return is_whitespace(c) || is_reserved(c) || c == '\0';
}

///---- Matching functions

/*
 * Match a keyword
 */
int match_keyword(const char *to_match, const char *c, int len)
{
    if (memcmp(to_match, c, len) == 0 && is_boundary(c[len]))
    {
        return len;
    }

    return 0;
}

/*
 * Match an identifier. Identifiers must begin with an alphabetic character,
 * and can contain any non-reserved character.
 */
int match_identifier(const char *c)
{
    int len = 0;

    // Identifiers must start with a letter, so we do some bounds checking
    if (*c < 'A')
        return 0;

    if (*c > 'Z' && *c < 'a')
        return 0;

    if (*c > 'z')
        return 0;

    while (*c != ' ' && *c != '\t' && *c != '\n' && *c != '\0')
    {
        if (is_reserved(*c))
            break;

        if (*c == '.')
            break;

        len = len + 1;
        c = c + 1;
    }

    return len;
}

/*
 * Match a number.
 */
int match_number(const char *c)
{
    int len = 0;

    while (!is_boundary(*c))
    {
        if (*c < '0' || *c > '9')
        {
            if (*c == '.' && *(c + 1) == '.')
                return len;
            return 0;
        }

        len = len + 1;
        c = c + 1;
    }

    return len;
}

/*
 * Match a floating point number.
 */
int match_float(const char *c)
{
    int len = 0;
    bool seen_dot = false;

    while (!is_whitespace(*c) && *c != 0 && !is_reserved(*c))
    {
        if ((*c < '0' || *c > '9') && *c != '.')
            return 0;

        if (*c == '.' && seen_dot == false)
            seen_dot = true;
        else if (*c == '.')
            return 0;

        len = len + 1;
        c = c + 1;
    }

    if (!seen_dot)
        return 0;

    return len;
}

/*
 * Match a string.
 */
int match_string(const char *c)
{
    int len = 1;

    if (*c != '"')
        return 0;

    do
    {
        len = len + 1;
        c = c + 1;
    } while (*c != '"');

    return len;
}

/*
 * Match the -> keyword.
 */
int match_right_arrow(const char *c)
{
    if (*c != '-')
        return 0;
    if (*(c + 1) != '>')
        return 0;

    return 2;
}

/*
 * Return the next token on the input stream, without advancing the scan position
 */
token_t peek(scan_context_t *context)
{
    int start;
    int position = context->position;
    int advance = 0;
    token_t t;
    bool token_found = true;

    // If our lookahead token has already been calculated, return it
    if (context->lookahead.start >= position && position > 0)
        return context->lookahead;

    do
    {
        char *c = context->buffer + position;

        start = position;
        // We don't set to true in the switch statement below
        token_found = true;
        switch (*c)
        {
        case ' ':
        case '\t':
            advance = 1;
            token_found = false;
            break;
        case '\0':
            t.type = TOK_EOF;
            advance = 1;
            break;
        case '\n':
            t.type = TOK_EOL;
            advance = 1;
            break;
        case '#':
            c += 1;
            advance = 1;
            while (*c != '\n')
            {
                c += 1;
                advance += 1;
            }
            token_found = false;
            break;
        case '=':
            if (*(c + 1) == '=')
            {
                t.type = TOK_EQUAL_EQUAL;
                advance = 2;
                break;
            }

            if (*(c + 1) == '>')
            {
                t.type = TOK_CASE;
                advance = 2;
                break;
            }

            t.type = TOK_EQUAL;
            advance = 1;
            break;
        case '!':
            if (*(c + 1) == '=')
            {
                t.type = TOK_BANG_EQUAL;
                advance = 2;
                break;
            }

            t.type = TOK_BANG;
            advance = 1;
            break;
        case '>':
            if (*(c + 1) == '=')
            {
                t.type = TOK_GREATER_EQUAL;
                advance = 2;
                break;
            }

            t.type = TOK_GREATER;
            advance = 1;
            break;
        case '<':
            if (*(c + 1) == '=')
            {
                t.type = TOK_LESS_EQUAL;
                advance = 2;
                break;
            }

            t.type = TOK_LESS;
            advance = 1;
            break;
        case '(':
            t.type = TOK_L_PAREN;
            advance = 1;
            break;
        case ')':
            t.type = TOK_R_PAREN;
            advance = 1;
            break;
        case '{':
            t.type = TOK_L_BRACE;
            advance = 1;
            break;
        case '}':
            t.type = TOK_R_BRACE;
            advance = 1;
            break;
        case ':':
            t.type = TOK_COLON;
            advance = 1;
            break;
        case ',':
            t.type = TOK_COMMA;
            advance = 1;
            break;
        case '-':
            advance = match_right_arrow(c);
            if (advance)
            {
                t.type = TOK_R_ARROW;
                break;
            }

            t.type = TOK_MINUS;
            advance = 1;
            break;
        case '+':
            t.type = TOK_PLUS;
            advance = 1;
            break;
        case '*':
            t.type = TOK_ASTERISK;
            advance = 1;
            break;
        case '/':
            t.type = TOK_SLASH;
            advance = 1;
            break;
        case '%':
            t.type = TOK_MODULO;
            advance = 1;
            break;
        case '_':
            t.type = TOK_WILDCARD_CASE;
            advance = 1;
            break;
        case '"':
            advance = match_string(c);
            if (advance)
            {
                t.type = TOK_STRING;
            }
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.':
            advance = match_number(c);
            if (advance)
            {
                t.type = TOK_NUMBER;
                break;
            }

            advance = match_float(c);
            if (advance)
            {
                t.type = TOK_FLOAT;
                break;
            }

            if (*c == '.' && *(c + 1) == '.')
            {
                t.type = TOK_DOT_DOT;
                advance = 2;
                break;
            }

            if (*c == '.')
            {
                t.type = TOK_DOT;
                advance = 1;
                break;
            }

            // We didn't match anything, so consume until we hit whitespace
            advance = 1;
            while (!is_whitespace(*(c + advance)))
            {
                advance = advance + 1;
            }
            t.type = TOK_INVALID;
            break;
        case 'a':
            advance = match_keyword("and", c, 3);
            if (advance)
            {
                t.type = TOK_AND;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }
            break;
        case 'e':
            advance = match_keyword("exported", c, 8);
            if (advance)
            {
                t.type = TOK_EXPORTED;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }

            break;
        case 'f':
            if (*(c + 1) == 'n')
            {
                advance = 2;
                t.type = TOK_FN;
                break;
            }

            advance = match_keyword("for", c, 3);
            if (advance)
            {
                t.type = TOK_FOR;
                break;
            }

            advance = match_keyword("false", c, 5);
            if (advance)
            {
                t.type = TOK_FALSE;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }
            break;
        case 'i':
            advance = match_keyword("if", c, 2);
            if (advance)
            {
                t.type = TOK_IF;
                break;
            }

            advance = match_keyword("in", c, 2);
            if (advance)
            {
                t.type = TOK_IN;
                break;
            }

            advance = match_keyword("import", c, 6);
            if (advance)
            {
                t.type = TOK_IMPORT;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }
            break;
        case 'l':
            advance = match_keyword("let", c, 3);
            if (advance)
            {
                t.type = TOK_LET;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }
            break;
        case 'n':
            advance = match_keyword("nil", c, 3);
            if (advance)
            {
                t.type = TOK_NIL;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }
            break;
        case 'm':
            advance = match_keyword("match", c, 5);
            if (advance)
            {
                t.type = TOK_MATCH;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }
            break;
        case 'o':
            advance = match_keyword("or", c, 2);
            if (advance)
            {
                t.type = TOK_OR;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }
            break;
        case 'r':
            advance = match_keyword("return", c, 6);
            if (advance)
            {
                t.type = TOK_RETURN;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }
            break;
        case 't':
            advance = match_keyword("true", c, 4);
            if (advance)
            {
                t.type = TOK_TRUE;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }
            break;
        case 'v':
            advance = match_keyword("var", c, 3);
            if (advance)
            {
                t.type = TOK_VAR;
                break;
            }

            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }
            break;
        default:
            advance = match_identifier(c);
            if (advance)
            {
                t.type = TOK_IDENTIFIER;
                break;
            }

            // We didn't match anything, so consume until we hit whitespace
            advance = 1;
            while (!is_whitespace(*(c + advance)))
            {
                advance = advance + 1;
            }
            t.type = TOK_INVALID;
        }

        // Advance our position
        position = start + advance;
    } while (!token_found);

    t.start = start;
    t.end = position;

    context->lookahead = t;

    return t;
}

/*
 * Accept the next token on the input stream, consuming it.
 */
token_t accept(scan_context_t *context)
{
    if (context->lookahead.start >= context->position && context->position > 0)
    {
        context->position = context->lookahead.end;
        context->previous = context->lookahead;
        return context->lookahead;
    }

    token_t t = peek(context);
    context->position = t.end;
    context->previous = t;
    return t;
}

void backup(scan_context_t *context)
{
    context->position = context->previous.start;
    context->lookahead = context->previous;
}

bool match(scan_context_t *context, int num, ...)
{
    bool found = false;
    token_t next = peek(context);

    va_list args;
    va_start(args, num);
    for (int i = 0; i < num; i++)
    {
        if (next.type == va_arg(args, int))
        {
            found = true;
            break;
        }
    }
    va_end(args);

    return found;
}

char *token_value(scan_context_t *context, token_t t)
{
    int len = t.end - t.start;
    char *value = calloc(len + 1, sizeof(char));

    // If the token is a string, omit the quotes
    if (t.type == TOK_STRING)
    {
        memcpy(value, context->buffer + t.start + 1, len - 2);
        value[len - 2] = 0;
    }
    else
    {
        memcpy(value, context->buffer + t.start, len);
    }

    return value;
}

/*
 * Scan a string, and return a list of corresponding tokens. This is primarily
 * for testing purposes.
 */
token_list_t scan_input(char *path, char *input)
{
    // We just create an arbitrarily-sized token list to begin with
    token_list_t tokens = token_list_create(2);
    token_t t;
    scan_context_t context = {path, input, 0};

    do
    {
        t = accept(&context);
        token_list_add(&tokens, t);
    } while (t.type != TOK_EOF);

    return tokens;
}
