/*
 * Copyright (c) 2021, Dana Burkart <dana.burkart@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

char *format_error(const char *listing_name, const char *listing, const char *str, location_t loc)
{
    char *error;
    int lineno = 1, position = 1, nl = 0;
    for (int i = 0; i < loc.start; i++)
    {
        if (listing[i] == '\n')
        {
            lineno++;
            position = i + 1;
        }
    }

    for (int i = position; nl == 0; i++)
    {
        if (listing[i] == '\n')
            nl = i;
    }

    char *line = malloc(nl - position + 1);
    memcpy(line, listing + position - 1, nl - position + 1);
    line[nl - position + 1] = 0;

    char *spacing = malloc(loc.start - position + 1);
    for (int i = 0; i <= loc.start - position; i++)
    {
        spacing[i] = ' ';
    }
    spacing[loc.start - position + 1] = 0;

    char *caret_pointer = malloc(loc.end - loc.start + 1);
    for (int i = 0; i < loc.end - loc.start; i++)
    {
        caret_pointer[i] = '^';
    }
    caret_pointer[loc.end - loc.start + 1] = 0;

    asprintf(&error,
             "%d:%llu: %s\n\n%s\n%s%s___ Found here.\n",
             lineno,
             loc.start - position,
             str,
             line,
             spacing,
             caret_pointer
            );
    return error;
}
