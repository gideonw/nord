/*
 * Copyright (c) 2021, Dana Burkart <dana.burkart@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <stdio.h>

#include "parse.h"
#include "util/error.h"

// Forward declarations
ast_t *statement_block(scan_context_t *);
ast_t *statement_list(scan_context_t *);
ast_t *case_block(scan_context_t *);
ast_t *case_list(scan_context_t *);
ast_t *statement(scan_context_t *);
ast_t *import_statement(scan_context_t *);
ast_t *if_statement(scan_context_t *);
ast_t *for_statement(scan_context_t *);
ast_t *match_statement(scan_context_t *);
ast_t *case_statement(scan_context_t *);
ast_t *function_decl(scan_context_t *);
ast_t *anonymous_decl(scan_context_t *);
ast_t *variable_decl(scan_context_t *);
ast_t *expression_list(scan_context_t *);
ast_t *expression(scan_context_t *);
ast_t *conjunction(scan_context_t *);
ast_t *equality(scan_context_t *);
ast_t *assignment(scan_context_t *);
ast_t *comparison(scan_context_t *);
ast_t *term(scan_context_t *);
ast_t *term_md(scan_context_t *);
ast_t *unary(scan_context_t *);
ast_t *primary(scan_context_t *);
ast_t *tuple(scan_context_t *);
ast_t *range(scan_context_t *);
ast_t *member_access(scan_context_t *);
ast_t *function_call(scan_context_t *);

ast_t *parse(scan_context_t *context)
{
    return statement_list(context);
}

void print_ast_internal(scan_context_t *context, ast_t *ast, int indent)
{
    char *token_val;

    if (ast == NULL)
        return;

    for (int i = 0; i < indent; i++)
    {
        if (i > 0 && i % 2 == 0)
            printf("⋅");
        else
            printf(" ");
    }

    if (indent > 0)
        printf("↳ ");

    switch (ast->type)
    {
    case AST_ASSIGN:
        token_val = ast->op.assign.name;
        printf("ASSIGN(IDENTIFIER) -> %s\n", token_val);
        print_ast_internal(context, ast->op.assign.value, indent + 2);
        break;
    case AST_BINARY:
        printf("BINARY(%s)\n", token_name(ast->op.binary.operator));
        print_ast_internal(context, ast->op.binary.left, indent + 2);
        print_ast_internal(context, ast->op.binary.right, indent + 2);
        break;
    case AST_DECLARE:
        token_val = ast->op.declare.name;
        printf("DECLARE(%s) -> %s\n", token_value(context, ast->op.declare.var_type), token_val);
        if (ast->op.declare.initial_value)
        {
            print_ast_internal(context, ast->op.declare.initial_value, indent + 2);
        }
        break;
    case AST_UNARY:
        printf("UNARY(%s)\n", token_name(ast->op.unary.operator));
        print_ast_internal(context, ast->op.unary.operand, indent + 2);
        break;
    case AST_LITERAL:
        printf("LITERAL(%s) -> %s\n", token_name(ast->op.literal.token), ast->op.literal.value);
        break;
    case AST_GROUP:
        printf("GROUP\n");
        print_ast_internal(context, ast->op.group, indent + 2);
        break;
    case AST_STMT_LIST:
        printf("STMT LIST\n");
        for (int i = 0; i < ast->op.list.size; i++)
        {
            print_ast_internal(context, ast->op.list.items[i], indent + 2);
        }
        break;
    case AST_FUNCTION_DECL:
        if (ast->op.fn.exported)
            printf("EXPORTED ");

        printf("FUNCTION_DECL(%s)\n", ast->op.fn.name);
        if (ast->op.fn.args != NULL)
        {
            print_ast_internal(context, ast->op.fn.args, indent + 2);
        }
        print_ast_internal(context, ast->op.fn.body, indent + 2);
        break;
    case AST_FUNCTION_CALL:
        printf("CALL_FN(%s)\n", ast->op.call.name);
        if (ast->op.call.args != NULL)
        {
            print_ast_internal(context, ast->op.call.args, indent + 2);
        }
        break;
    case AST_VAR_LIST:
    case AST_EXPR_LIST:
        printf("ARGUMENTS\n");
        for (int i = 0; i < ast->op.list.size; i++)
        {
            print_ast_internal(context, ast->op.list.items[i], indent + 2);
        }
        break;

    case AST_TUPLE:
        printf("TUPLE\n");
        for (int i = 0; i < ast->op.list.size; i++)
        {
            print_ast_internal(context, ast->op.list.items[i], indent + 2);
        }
        break;

    case AST_IF_STMT:
        printf("IF\n");
        print_ast_internal(context, ast->op.if_stmt.condition, indent + 2);
        print_ast_internal(context, ast->op.if_stmt.body, indent + 2);
        break;

    case AST_FOR_STMT:
        if (ast->op.for_stmt.var != NULL)
        {
            printf("FOR(%s)\n", ast->op.for_stmt.var);
        }
        else
        {
            printf("FOR\n");
        }
        print_ast_internal(context, ast->op.for_stmt.iterable, indent + 2);
        print_ast_internal(context, ast->op.for_stmt.body, indent + 2);
        break;

    case AST_MATCH_STMT:
        if (ast->op.match_stmt.var != NULL)
        {
            printf("MATCH(%s)\n", ast->op.match_stmt.var);
        }
        else
        {
            printf("MATCH\n");
        }
        print_ast_internal(context, ast->op.match_stmt.match_op, indent + 2);
        print_ast_internal(context, ast->op.match_stmt.body, indent + 2);
        break;

    case AST_CASE_LIST:
        printf("CASE LIST\n");
        for (int i = 0; i < ast->op.list.size; i++)
        {
            print_ast_internal(context, ast->op.list.items[i], indent + 2);
        }
        break;

    case AST_CASE_STMT:
        printf("CASE\n");
        print_ast_internal(context, ast->op.case_stmt.condition, indent + 2);
        print_ast_internal(context, ast->op.case_stmt.body, indent + 2);
        break;

    case AST_RANGE:
        printf("RANGE\n");
        print_ast_internal(context, ast->op.range.begin, indent + 2);
        print_ast_internal(context, ast->op.range.end, indent + 2);
        break;

    case AST_MODULE:
        printf("IMPORT %s\n", ast->op.module.name);
        break;
    }
}

void print_ast(scan_context_t *context, ast_t *ast)
{
    print_ast_internal(context, ast, 0);
}

ast_t *make_assign_expr(char *name, ast_t *value)
{
    ast_t *assign_expr = (ast_t *)malloc(sizeof(ast_t));
    assign_expr->type = AST_ASSIGN;
    assign_expr->op.assign.name = name;
    assign_expr->op.assign.value = value;

    return assign_expr;
}

ast_t *make_binary_expr(ast_t *left, token_t operator, ast_t * right)
{
    ast_t *binary_expr = (ast_t *)malloc(sizeof(ast_t));
    binary_expr->type = AST_BINARY;
    binary_expr->op.binary.operator= operator;
    binary_expr->op.binary.left = left;
    binary_expr->op.binary.right = right;

    return binary_expr;
}

ast_t *make_declare_expr(token_t var_type, char *name, ast_t *initial_value)
{
    ast_t *declare_expr = (ast_t *)malloc(sizeof(ast_t));
    declare_expr->type = AST_DECLARE;
    declare_expr->op.declare.var_type = var_type;
    declare_expr->op.declare.name = name;
    declare_expr->op.declare.initial_value = initial_value;

    return declare_expr;
}

ast_t *make_unary_expr(token_t operator, ast_t * operand)
{
    ast_t *unary_expr = (ast_t *)malloc(sizeof(ast_t));
    unary_expr->type = AST_UNARY;
    unary_expr->op.unary.operator= operator;
    unary_expr->op.unary.operand = operand;

    return unary_expr;
}

ast_t *make_literal_expr(token_t literal)
{
    ast_t *literal_expr = (ast_t *)malloc(sizeof(ast_t));
    literal_expr->type = AST_LITERAL;
    literal_expr->op.literal.token = literal;

    return literal_expr;
}

ast_t *make_group_expr(ast_t *expr)
{
    ast_t *group_expr = (ast_t *)malloc(sizeof(ast_t));
    group_expr->type = AST_GROUP;
    group_expr->op.group = expr;

    return group_expr;
}

ast_t *make_fn_expr(char *name, bool exported, ast_t *args, ast_t *body)
{
    ast_t *fn_expr = (ast_t *)malloc(sizeof(ast_t));
    fn_expr->type = AST_FUNCTION_DECL;
    fn_expr->op.fn.name = name;
    fn_expr->op.fn.exported = exported;
    fn_expr->op.fn.args = args;
    fn_expr->op.fn.body = body;
    return fn_expr;
}

ast_t *make_call_expr(char *name, ast_t *args)
{
    ast_t *call_expr = (ast_t *)malloc(sizeof(ast_t));
    call_expr->type = AST_FUNCTION_CALL;
    call_expr->op.call.name = name;
    call_expr->op.call.args = args;
    return call_expr;
}

// List handling
ast_t *make_list_expr(size_t capacity)
{
    ast_t *list_expr = (ast_t *)malloc(sizeof(ast_t));
    list_expr->type = AST_STMT_LIST;
    list_expr->op.list.size = 0;
    list_expr->op.list.capacity = capacity;
    list_expr->op.list.items = (ast_t **)malloc(sizeof(ast_t) * capacity);
    return list_expr;
}

ast_t *make_case_list_expr(size_t capacity)
{
    ast_t *list_expr = (ast_t *)malloc(sizeof(ast_t));
    list_expr->type = AST_CASE_LIST;
    list_expr->op.list.size = 0;
    list_expr->op.list.capacity = capacity;
    list_expr->op.list.items = (ast_t **)malloc(sizeof(ast_t) * capacity);
    return list_expr;
}

ast_t *make_if_expr(ast_t *condition, ast_t *body)
{
    ast_t *if_expr = (ast_t *)malloc(sizeof(ast_t));
    if_expr->type = AST_IF_STMT;
    if_expr->op.if_stmt.condition = condition;
    if_expr->op.if_stmt.body = body;
    return if_expr;
}

ast_t *make_for_expr(char *var, ast_t *iterable, ast_t *body)
{
    ast_t *for_expr = (ast_t *)malloc(sizeof(ast_t));
    for_expr->type = AST_FOR_STMT;
    for_expr->op.for_stmt.var = var;
    for_expr->op.for_stmt.iterable = iterable;
    for_expr->op.for_stmt.body = body;
    return for_expr;
}

ast_t *make_match_expr(char *var, ast_t *match_op, ast_t *body)
{
    ast_t *match_expr = (ast_t *)malloc(sizeof(ast_t));
    match_expr->type = AST_MATCH_STMT;
    match_expr->op.match_stmt.var = var;
    match_expr->op.match_stmt.match_op = match_op;
    match_expr->op.match_stmt.body = body;
    return match_expr;
}

ast_t *make_case_expr(ast_t *condition, ast_t *body)
{
    ast_t *match_expr = (ast_t *)malloc(sizeof(ast_t));
    match_expr->type = AST_CASE_STMT;
    match_expr->op.case_stmt.condition = condition;
    match_expr->op.case_stmt.body = body;
    return match_expr;
}

ast_t *make_range_expr(ast_t *begin, ast_t *end)
{
    ast_t *range_expr = (ast_t *)malloc(sizeof(ast_t));
    range_expr->type = AST_RANGE;
    range_expr->op.range.begin = begin;
    range_expr->op.range.end = end;
    return range_expr;
}

ast_t *make_module_expr(char *module_name)
{
    ast_t *module_expr = (ast_t *)malloc(sizeof(ast_t));
    module_expr->type = AST_MODULE;
    module_expr->op.module.name = module_name;
    return module_expr;
}

void list_expr_append(ast_t *list, ast_t *item)
{
    if (list->op.list.size >= list->op.list.capacity - 1)
    {
        list->op.list.capacity *= 2;
        list->op.list.items = (ast_t **)realloc(list->op.list.items, sizeof(ast_t) * list->op.list.capacity);
    }

    list->op.list.items[list->op.list.size++] = item;
}

ast_t *statement_block(scan_context_t *context)
{
    ast_t *left;
    bool rewind = false;

    // Allow up to one EOL before the brace to account for coding style
    // preferences.
    if (peek(context).type == TOK_EOL)
    {
        accept(context);
        rewind = true;
    }

    if (peek(context).type != TOK_L_BRACE)
    {
        backup(context);
        return NULL;
    }

    accept(context);

    while (peek(context).type == TOK_EOL)
        accept(context);

    left = statement_list(context);

    // TODO: Proper error handling
    assert(left != NULL);

    token_t invalid;
    if ((invalid = accept(context)).type != TOK_R_BRACE)
    {
        char *error;
        location_t loc = {invalid.start, invalid.end};
        asprintf(&error, "Expected closing brace of statement block (\"}\").");
        printf("%s", format_error(context->name, context->buffer, error, loc));
        exit(1);
    }

    return left;
}

ast_t *statement_list(scan_context_t *context)
{
    ast_t *statements = make_list_expr(10);

    ast_t *current = statement(context);

    if (current == NULL)
    {
        return statements;
    }

    list_expr_append(statements, current);

    while (peek(context).type != TOK_EOF && peek(context).type != TOK_EOF)
    {
        // If the next token is and EOL, consume it
        if (peek(context).type == TOK_EOL)
            accept(context);

        // Now pull off the next statement
        current = statement(context);

        if (current != NULL)
            list_expr_append(statements, current);
        else
            break;
    }

    return statements;
}

ast_t *statement(scan_context_t *context)
{
    ast_t *left = variable_decl(context);

    if (peek(context).type == TOK_RETURN)
    {
        token_t ret = accept(context);
        return make_unary_expr(ret, statement(context));
    }

    if (left == NULL)
        left = expression(context);

    if (left == NULL)
        left = function_decl(context);

    if (left == NULL)
        left = anonymous_decl(context);

    if (left == NULL)
        left = if_statement(context);

    if (left == NULL)
        left = for_statement(context);

    if (left == NULL)
        left = match_statement(context);

    if (left == NULL)
        left = import_statement(context);

    if (peek(context).type == TOK_EOL)
    {
        accept(context);
        if (left == NULL)
            return statement(context);
        else
            return left;
    }

    if (peek(context).type == TOK_EOF)
        return left;

    return left;
}

ast_t *case_block(scan_context_t *context)
{
    ast_t *left;
    bool rewind = false;

    // Allow up to one EOL before the brace to account for coding style
    // preferences.
    if (peek(context).type == TOK_EOL)
    {
        accept(context);
        rewind = true;
    }

    if (peek(context).type != TOK_L_BRACE)
    {
        backup(context);
        return NULL;
    }

    accept(context);

    while (peek(context).type == TOK_EOL)
        accept(context);

    left = case_list(context);

    // TODO: Proper error handling
    assert(left != NULL);

    token_t invalid;
    if ((invalid = accept(context)).type != TOK_R_BRACE)
    {
        char *error;
        location_t loc = {invalid.start, invalid.end};
        asprintf(&error, "Expected closing brace of case block (\"}\").");
        printf("%s", format_error(context->name, context->buffer, error, loc));
        exit(1);
    }

    return left;
}

ast_t *case_list(scan_context_t *context)
{
    ast_t *statements = make_case_list_expr(10);

    ast_t *current = case_statement(context);

    if (current == NULL)
    {
        return statements;
    }

    list_expr_append(statements, current);

    while (peek(context).type != TOK_EOF && peek(context).type != TOK_EOF)
    {
        // If the next token is and EOL, consume it
        if (peek(context).type == TOK_EOL)
            accept(context);

        // Now pull off the next statement
        current = case_statement(context);

        if (current != NULL)
            list_expr_append(statements, current);
        else
            break;
    }

    return statements;
}

ast_t *case_statement(scan_context_t *context)
{
    // match a primary
    if (match(context, 5, TOK_WILDCARD_CASE, TOK_STRING, TOK_NUMBER, TOK_TRUE, TOK_FALSE))
    {
        // accept what we are matching
        ast_t *condition = primary(context);
        if (peek(context).type != TOK_CASE)
        {
            backup(context);
            return NULL;
        }

        // Pull off the "case" operator
        token_t case_kw = accept(context);
        assert(case_kw.type == TOK_CASE);

        if (condition == NULL)
        {
            char *error;
            accept(context);
            location_t loc = {case_kw.end, case_kw.end + 1};
            asprintf(&error, "Expected primary before case operator.");
            printf("%s", format_error(context->name, context->buffer, error, loc));
            exit(1);
        }

        ast_t *body = statement_block(context);

        if (body == NULL)
        {
            body = statement(context);
        }

        if (body == NULL)
        {
            char *error;
            accept(context);
            location_t loc = {condition->location.end, condition->location.end + 1};
            asprintf(&error, "Expected statement or body following case-statement.");
            // TODO: Refactor error handling to handle custom "Found here." text
            printf("%s", format_error(context->name, context->buffer, error, loc));
            exit(1);
        }

        return make_case_expr(condition, body);
    }

    // not asure what the error case is here, maybe check for a catch all?
    // assert(true == false);
}

ast_t *import_statement(scan_context_t *context)
{
    if (peek(context).type != TOK_IMPORT)
        return NULL;

    accept(context);

    if (peek(context).type != TOK_STRING)
    {
        char *error;
        token_t invalid = accept(context);
        location_t loc = {invalid.start, invalid.end};
        asprintf(&error, "Expected string following import.");
        printf("%s", format_error(context->name, context->buffer, error, loc));
        exit(1);
    }

    token_t module_name = accept(context);

    return make_module_expr(token_value(context, module_name));
}

ast_t *if_statement(scan_context_t *context)
{
    if (peek(context).type != TOK_IF)
        return NULL;

    // Pull off the "if" keyword
    token_t if_kw = accept(context);

    ast_t *condition = expression(context);

    if (condition == NULL)
    {
        char *error;
        accept(context);
        location_t loc = {if_kw.end, if_kw.end + 1};
        asprintf(&error, "Expected expression following if keyword.");
        printf("%s", format_error(context->name, context->buffer, error, loc));
        exit(1);
    }

    ast_t *body = statement_block(context);

    if (body == NULL)
    {
        body = statement(context);
    }

    if (body == NULL)
    {
        char *error;
        accept(context);
        location_t loc = {condition->location.end, condition->location.end + 1};
        asprintf(&error, "Expected statement or body following if-statement.");
        // TODO: Refactor error handling to handle custom "Found here." text
        printf("%s", format_error(context->name, context->buffer, error, loc));
        exit(1);
    }

    return make_if_expr(condition, body);
}

ast_t *for_statement(scan_context_t *context)
{
    char *var = NULL;
    ast_t *iterable = NULL;
    ast_t *body = NULL;

    if (peek(context).type != TOK_FOR)
        return NULL;

    token_t for_kw = accept(context);

    // Are we defining a local for each iteration?
    if (peek(context).type == TOK_IDENTIFIER)
    {
        token_t t = accept(context);

        if (peek(context).type == TOK_IN)
        {
            var = token_value(context, t);
            accept(context);
            iterable = primary(context);
        }
        else
        {
            backup(context);
            iterable = primary(context);
        }
    }

    if (iterable == NULL && match(context, 3, TOK_STRING, TOK_L_PAREN, TOK_NUMBER))
    {
        iterable = primary(context);
    }

    if (iterable == NULL)
    {
        char *error;
        token_t invalid = accept(context);
        location_t loc = {for_kw.end, invalid.start};
        asprintf(&error, "Expected iterable type after \"for\" keyword.");
        // TODO: Refactor error handling to handle custom "Found here." text
        printf("%s", format_error(context->name, context->buffer, error, loc));
        exit(1);
    }

    body = statement_block(context);

    if (body == NULL)
    {
        char *error;
        accept(context);
        location_t loc = {iterable->location.end, iterable->location.end + 1};
        asprintf(&error, "Expected statement or body following for statement.");
        // TODO: Refactor error handling to handle custom "Found here." text
        printf("%s", format_error(context->name, context->buffer, error, loc));
        exit(1);
    }

    return make_for_expr(var, iterable, body);
}

ast_t *match_statement(scan_context_t *context)
{
    char *var = NULL;
    ast_t *match_op = NULL;
    ast_t *body = NULL;

    if (peek(context).type != TOK_MATCH)
        return NULL;

    token_t match_kw = accept(context);

    token_t peek_type = peek(context);
    if (peek_type.type == TOK_IDENTIFIER)
    {
        token_t t = accept(context);
        var = token_value(context, t);
        accept(context);
        match_op = primary(context);
    }

    if (match_op == NULL && match(context, 3, TOK_STRING, TOK_L_PAREN, TOK_NUMBER))
    {
        match_op = primary(context);
    }

    if (match_op == NULL)
    {
        char *error;
        token_t invalid = accept(context);
        location_t loc = {match_kw.end, invalid.start};
        asprintf(&error, "Expected comparable type after \"match\" keyword, found %s", token_name(peek_type));
        // TODO: Refactor error handling to handle custom "Found here." text
        printf("%s", format_error(context->name, context->buffer, error, loc));
        exit(1);
    }

    body = case_block(context);

    if (body == NULL)
    {
        char *error;
        accept(context);
        location_t loc = {match_op->location.end, match_op->location.end + 1};
        asprintf(&error, "Expected statement or body following match statement.");
        // TODO: Refactor error handling to handle custom "Found here." text
        printf("%s", format_error(context->name, context->buffer, error, loc));
        exit(1);
    }

    return make_match_expr(var, match_op, body);
}

ast_t *function_decl(scan_context_t *context)
{
    ast_t *left, *args = NULL, *body;
    char *name;
    bool exported = false;

    if (peek(context).type == TOK_SLASH)
    {
        accept(context);
        if (peek(context).type != TOK_EXPORTED)
        {
            backup(context);
            return NULL;
        }
        accept(context);
        exported = true;

        // TODO: Handle error
        assert(accept(context).type == TOK_SLASH);

        // TODO: Consume N newlines
        if (peek(context).type == TOK_EOL)
            accept(context);
    }

    if (peek(context).type != TOK_FN)
        return NULL;

    accept(context);

    if (peek(context).type == TOK_IDENTIFIER)
    {
        name = token_value(context, accept(context));
    }
    else
    {
        backup(context);
        return NULL;
    }

    if (peek(context).type == TOK_L_PAREN)
    {
        accept(context);
        args = expression_list(context);
        // TODO: Handle error
        assert(accept(context).type == TOK_R_PAREN);
    }

    body = statement_block(context);
    // TODO: Handle error
    assert(body != NULL);

    left = make_fn_expr(name, exported, args, body);

    return left;
}

ast_t *anonymous_decl(scan_context_t *context)
{
    ast_t *left, *args = NULL, *body;
    char *name;

    if (peek(context).type != TOK_FN)
        return NULL;

    accept(context);

    if (peek(context).type == TOK_L_BRACE || peek(context).type == TOK_L_PAREN)
    {
        name = "__anonymous";
    }
    else
    {
        backup(context);
        return NULL;
    }

    if (peek(context).type == TOK_L_PAREN)
    {
        accept(context);
        args = expression_list(context);
        // TODO: Handle error
        assert(accept(context).type == TOK_R_PAREN);
    }

    body = statement_block(context);
    // TODO: Handle error
    assert(body != NULL);

    left = make_fn_expr(name, false, args, body);

    return left;
}

ast_t *variable_decl(scan_context_t *context)
{
    ast_t *left;

    if (peek(context).type != TOK_VAR && peek(context).type != TOK_LET)
        return NULL;

    token_t var_type = accept(context);

    if (peek(context).type != TOK_IDENTIFIER)
    {
        char *error;
        token_t invalid = accept(context);
        location_t loc = {invalid.start, invalid.end};
        asprintf(&error, "Expected identifier in declaration, but found \"%s\".", token_value(context, invalid));
        printf("%s", format_error(context->name, context->buffer, error, loc));
        exit(1);
    }

    token_t name = accept(context);
    ast_t *right = NULL;

    if (peek(context).type == TOK_EQUAL)
    {
        accept(context);
        right = expression(context);
    }

    left = make_declare_expr(var_type, token_value(context, name), right);
    left->location.start = var_type.start;
    left->location.end = (right) ? right->location.end : name.end;

    return left;
}

ast_t *expression_list(scan_context_t *context)
{
    ast_t *expr = expression(context);

    if (expr == NULL)
        return NULL;

    ast_t *left = make_list_expr(10);

    list_expr_append(left, expr);
    left->type = AST_EXPR_LIST;

    while (peek(context).type == TOK_COMMA)
    {
        // Pull off the comma
        accept(context);

        expr = expression(context);
        // TODO: Handle error
        assert(expr != NULL);
        list_expr_append(left, expr);
    }

    return left;
}

ast_t *expression(scan_context_t *context)
{
    ast_t *value = assignment(context);

    if (value == NULL)
    {
        value = anonymous_decl(context);
    }

    return value;
}

ast_t *assignment(scan_context_t *context)
{
    ast_t *left = NULL;
    token_t name;

    if (peek(context).type != TOK_IDENTIFIER)
        return conjunction(context);

    name = accept(context);

    if (peek(context).type != TOK_EQUAL)
    {
        free(left);
        backup(context);
        return conjunction(context);
    }

    // Consume the '='
    accept(context);

    ast_t *value = expression(context);
    left = make_assign_expr(token_value(context, name), value);
    left->location.start = name.start;
    left->location.end = name.end;

    return left;
}

ast_t *conjunction(scan_context_t *context)
{
    ast_t *left = equality(context);

    while (match(context, 2, TOK_AND, TOK_OR))
    {
        token_t operator= accept(context);
        ast_t *right = equality(context);
        ast_t *new_left = make_binary_expr(left, operator, right);
        new_left->location.start = left->location.start;
        new_left->location.end = right->location.end;
        left = new_left;
    }

    return left;
}

ast_t *equality(scan_context_t *context)
{
    ast_t *left = comparison(context);

    while (match(context, 2, TOK_BANG_EQUAL, TOK_EQUAL_EQUAL))
    {
        token_t operator= accept(context);
        ast_t *right = comparison(context);
        ast_t *new_left = make_binary_expr(left, operator, right);
        new_left->location.start = left->location.start;
        new_left->location.end = right->location.end;
        left = new_left;
    }

    return left;
}

ast_t *comparison(scan_context_t *context)
{
    ast_t *left = term(context);

    while (match(context, 4, TOK_GREATER, TOK_GREATER_EQUAL, TOK_LESS, TOK_LESS_EQUAL))
    {
        token_t operator= accept(context);
        ast_t *right = term(context);
        ast_t *new_left = make_binary_expr(left, operator, right);
        new_left->location.start = left->location.start;
        new_left->location.end = right->location.end;
        left = new_left;
    }

    return left;
}

ast_t *term(scan_context_t *context)
{
    ast_t *left = term_md(context);

    while (match(context, 3, TOK_MINUS, TOK_PLUS, TOK_MODULO))
    {
        token_t operator= accept(context);
        ast_t *right = term_md(context);
        ast_t *new_left = make_binary_expr(left, operator, right);
        new_left->location.start = left->location.start;
        new_left->location.end = right->location.end;
        left = new_left;
    }

    return left;
}

ast_t *term_md(scan_context_t *context)
{
    ast_t *left = unary(context);

    while (match(context, 2, TOK_SLASH, TOK_ASTERISK))
    {
        token_t operator= accept(context);

        if (operator.type == TOK_SLASH && peek(context).type == TOK_EXPORTED)
        {
            backup(context);
            return NULL;
        }
        ast_t *right = unary(context);
        ast_t *new_left = make_binary_expr(left, operator, right);
        new_left->location.start = left->location.start;
        new_left->location.end = right->location.end;
        left = new_left;
    }

    return left;
}

ast_t *unary(scan_context_t *context)
{
    if (match(context, 2, TOK_BANG, TOK_MINUS))
    {
        token_t operator= accept(context);
        ast_t *operand = unary(context);
        ast_t *unary = make_unary_expr(operator, operand);
        unary->location.start = operator.start;
        unary->location.end = operand->location.end;
        return unary;
    }
    else
    {
        ast_t *p = primary(context);
        if (p == NULL && peek(context).type == TOK_INVALID)
        {
            char *error;
            location_t loc = {peek(context).start, peek(context).end};
            asprintf(&error, "Unexpected token. Expected keyword, number, string, or identifier, but found \"%s\"", token_value(context, peek(context)));
            printf("%s", format_error(context->name, context->buffer, error, loc));
            exit(1);
        }
        return p;
    }
}

ast_t *primary(scan_context_t *context)
{
    ast_t *left = function_call(context);
    if (left != NULL)
        return left;

    left = member_access(context);
    if (left != NULL)
        return left;

    left = range(context);
    if (left != NULL)
        return left;

    if (match(context, 8, TOK_IDENTIFIER, TOK_NUMBER, TOK_FLOAT, TOK_STRING, TOK_TRUE, TOK_FALSE, TOK_NIL, TOK_WILDCARD_CASE))
    {
        token_t tok = accept(context);
        ast_t *literal = make_literal_expr(tok);
        literal->op.literal.value = token_value(context, literal->op.literal.token);
        literal->location.start = tok.start;
        literal->location.end = tok.end;
        return literal;
    }

    return tuple(context);
}

ast_t *tuple(scan_context_t *context)
{
    if (peek(context).type == TOK_L_PAREN)
    {
        // Consume the parenthesis
        token_t paren = accept(context);

        ast_t *expr = expression_list(context);
        // TODO: Error checking!
        assert(expr != NULL);
        expr->location.start = paren.start;
        paren = accept(context);
        expr->location.end = paren.end;

        if (expr->op.list.size == 1)
        {
            ast_t *tmp = make_group_expr(expr->op.list.items[0]);
            free(expr->op.list.items);
            free(expr);
            expr = tmp;
        }
        else
        {
            expr->type = AST_TUPLE;
        }

        if (paren.type != TOK_R_PAREN)
        {
            char *error;
            location_t loc = {paren.start, paren.end};
            asprintf(&error, "Mismatched parenthesis. Expected \")\", but found \"%s\".", token_value(context, paren));
            printf("%s", format_error(context->name, context->buffer, error, loc));
            exit(1);
        }

        return expr;
    }

    return NULL;
}

ast_t *range(scan_context_t *context)
{
    ast_t *range = NULL;
    ast_t *begin, *end;

    if (match(context, 2, TOK_IDENTIFIER, TOK_NUMBER))
    {
        token_t tok = accept(context);

        if (peek(context).type != TOK_DOT_DOT)
        {
            backup(context);
            return NULL;
        }

        begin = make_literal_expr(tok);
        begin->op.literal.value = token_value(context, tok);

        accept(context);

        // TODO: Error handling!
        assert(match(context, 2, TOK_IDENTIFIER, TOK_NUMBER));

        tok = accept(context);
        end = make_literal_expr(tok);
        end->op.literal.value = token_value(context, tok);
        range = make_range_expr(begin, end);
    }

    return range;
}

ast_t *member_access(scan_context_t *context)
{
    ast_t *left;

    if (peek(context).type != TOK_IDENTIFIER)
        return NULL;

    token_t identifier = accept(context);
    left = make_literal_expr(identifier);
    left->op.literal.value = token_value(context, identifier);
    left->location.start = identifier.start;
    left->location.end = identifier.end;

    if (peek(context).type != TOK_DOT)
    {
        backup(context);
        return NULL;
    }

    token_t operator= accept(context);

    ast_t *right = function_call(context);
    if (right == NULL)
    {
        right = member_access(context);
    }

    return make_binary_expr(left, operator, right);
}

ast_t *function_call(scan_context_t *context)
{
    ast_t *left;
    ast_t *args;
    char *fn_name;

    if (peek(context).type != TOK_IDENTIFIER)
        return NULL;

    token_t identifier = accept(context);
    fn_name = token_value(context, identifier);

    if (peek(context).type != TOK_L_PAREN)
    {
        backup(context);
        return NULL;
    }

    accept(context);

    args = expression_list(context);

    // TODO: Error handling
    assert(accept(context).type == TOK_R_PAREN);

    left = make_call_expr(fn_name, args);
    left->location.start = identifier.start;
    left->location.end = context->position;

    return left;
}
