/*
 * Copyright (c) 2021, Dana Burkart <dana.burkart@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

bool is_collection(value_t value)
{
    return value.type == VAL_STRING || value.type == VAL_TUPLE;
}

value_t iterator_create(value_t collection)
{
    // TODO: Handle errors
    assert(is_collection(collection));

    value_t val;
    iterator_t *iter = (iterator_t *)malloc(sizeof(iterator_t));

    iter->object.type = VAL_ITERATOR;
    iter->iterable = collection;
    iter->index = 0;

    val.type = VAL_ITERATOR;
    val.contents.object = (object_t *)iter;

    return val;
}

value_t string_create(char *string)
{
    value_t val;
    string_t *str = (string_t *)malloc(sizeof(string_t));

    str->object.type = VAL_STRING;
    str->length = strlen(string);
    str->string = string;

    val.type = VAL_STRING;
    val.contents.object = (object_t *)str;

    return val;
}

value_t tuple_create(int length)
{
    value_t val;
    tuple_t *tuple = (tuple_t *)malloc(sizeof(tuple_t));

    tuple->object.type = VAL_TUPLE;
    tuple->length = length;
    tuple->values = (value_t *)malloc(sizeof(value_t) * length);

    val.type = VAL_TUPLE;
    val.contents.object = (object_t *)tuple;

    return val;
}
