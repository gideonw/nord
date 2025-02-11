/*
 * Copyright (c) 2021, Dana Burkart <dana.burkart@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compile.h"
#include "lang/module.h"
#include "machine/bytecode.h"
#include "machine/value.h"
#include "util/error.h"
#include "util/macros.h"

#define INSTRUCTION(...) VFUNC(INSTRUCTION, __VA_ARGS__)

#define INSTRUCTION4(OP, ARG1, ARG2, ARG3) (instruction_t){ OP, .fields={ .triplet={ARG1, ARG2, ARG3} } }
#define INSTRUCTION3(OP, ARG1, ARG2) (instruction_t){ OP, .fields={ .pair={ARG1, ARG2 } } }
#define INSTRUCTION2(OP, ARG1) INSTRUCTION3(OP, 0, ARG1)

typedef struct
{
    // Name of the module we are compiling
    const char *name;
    // Source code that is being compiled
    const char *listing;
    symbol_map_t *symbols;
    binary_t *binary;
    code_block_t *current_code_block;
    uint8_t rp;
    uint64_t mp;
    uint64_t cp;
    symbol_t member_context;
} compile_context_t;

typedef struct
{
    uint8_t location;
    value_type_e type;
    code_block_t *code;
} compile_result_t;

compile_result_t compile_ast(ast_t *ast, compile_context_t *context);

compile_context_t *context_create(const char *name, const char *listing)
{
    compile_context_t *context = malloc(sizeof(compile_context_t));

    context->name = name;
    context->listing = listing;
    context->symbols = symbol_map_create();
    context->binary = binary_create();
    context->binary->data = memory_create(1);
    context->binary->code = code_collection_create();
    context->binary->symbols = symbol_map_create();
    context->current_code_block = code_block_create();
    context->member_context = (symbol_t){ 0 };
    code_collection_add_block(context->binary->code, context->current_code_block);
    context->rp = 1;
    context->cp = 0;

    // Set up true and false
    memory_set(context->binary->data, 0, (value_t){VAL_BOOLEAN, false});
    memory_set(context->binary->data, 1, (value_t){VAL_BOOLEAN, true});

    context->mp = 2;

    return context;
}

void context_destroy(compile_context_t *context)
{
    symbol_map_destroy(context->symbols);
    // NOTE: We don't free the code block, since that is returned (and this is
    //       an internal data structure).
    free(context);
}

static inline compile_result_t write_out_builtin(compile_context_t *context, char *name, uint8_t nargs, uint8_t *args)
{
    symbol_t fn_symbol = symbol_map_get(context->symbols, name);

    if (fn_symbol.location.type == LOC_UNDEF)
    {
        memory_set(context->binary->data, context->mp, string_create(name));
        fn_symbol.location.type = LOC_BUILTIN;
        fn_symbol.location.address = context->mp++;
        fn_symbol.name = name;
        fn_symbol.type = SYM_FN;

        symbol_map_t *map = context->symbols;
        while (map->parent != NULL)
            map = map->parent;
        symbol_map_set(map, fn_symbol);
    }

    uint8_t reset_register = context->rp;

    // We push args onto the stack in reverse order because we'll pop them
    // off in the builtin
    if (nargs)
    {
        for (int i = nargs - 1; i >= 0; i--)
        {
            code_block_write(context->current_code_block, INSTRUCTION(OP_PUSH, args[i]));
        }
    }

    context->rp = reset_register;

    // Number of args -> $0
    code_block_write(context->current_code_block, INSTRUCTION(OP_LOADV, 0, nargs));
    code_block_write(context->current_code_block, INSTRUCTION(OP_CALL_DYNAMIC, fn_symbol.location.address));
    code_block_write(context->current_code_block, INSTRUCTION(OP_POP, context->rp));

    return (compile_result_t){ .location=context->rp, .type=VAL_UNKNOWN, .code=NULL };
}

//-- Compile AST nodes

compile_result_t compile_statement_list(ast_t *ast, compile_context_t *context)
{
    // Create our inner scope for this block
    symbol_map_t *inner_scope = symbol_map_create();
    inner_scope->parent = context->symbols;
    context->symbols = inner_scope;

    // Create a new code block for this block
    code_block_t *previous_block =  context->current_code_block;
    code_block_t *inner_block = code_block_create();
    context->current_code_block = inner_block;

    compile_result_t result;

    // If it's not a statement list, simply compile the one statement
    if (ast->type != AST_STMT_LIST)
    {
        result = compile_ast(ast, context);
    }
    else
    {
        for (int i = 0; i < ast->op.list.size; i++)
        {
            result = compile_ast(ast->op.list.items[i], context);
        }
    }

    // Restore code block
    context->current_code_block = previous_block;

    // Restore scope
    context->symbols = context->symbols->parent;
    symbol_map_destroy(inner_scope);

    // Modify result to include our generated block
    result.code = inner_block;
    return result;
}

compile_result_t compile_literal(ast_t *ast, compile_context_t *context)
{
    value_type_e type = VAL_UNKNOWN;
    uint8_t result = context->rp;
    switch (ast->op.literal.token.type)
    {
        case TOK_NUMBER:
            code_block_write(context->current_code_block, INSTRUCTION(OP_LOADV, context->rp, atoi(ast->op.literal.value)));
            type = VAL_INT;
            break;

        case TOK_STRING:
            memory_set(context->binary->data, context->mp, value(ast->op.literal.value));
            code_block_write(context->current_code_block, INSTRUCTION(OP_LOAD, context->rp, context->mp++));
            type = VAL_STRING;
            break;

        case TOK_FLOAT:
            memory_set(context->binary->data, context->mp, value(atof(ast->op.literal.value)));
            code_block_write(context->current_code_block, INSTRUCTION(OP_LOAD, context->rp, context->mp++));
            type = VAL_FLOAT;
            break;

        case TOK_TRUE:
        case TOK_FALSE:
            code_block_write(context->current_code_block, INSTRUCTION(OP_LOAD, context->rp, (ast->op.literal.token.type == TOK_TRUE) ? 1 : 0));
            type = VAL_BOOLEAN;
            break;

        case TOK_NIL:
            code_block_write(context->current_code_block, INSTRUCTION(OP_NIL, context->rp));
            type = VAL_NIL;
            break;

        case TOK_IDENTIFIER:
            {
                symbol_t identifier = symbol_map_get(context->symbols, ast->op.literal.value);
                if (identifier.location.type == LOC_UNDEF)
                {
                    char *error;
                    location_t loc = {ast->location.start, ast->location.end};
                    asprintf(&error, "Use of undeclared identifier \"%s\"", ast->op.literal.value);
                    printf("%s", format_error(context->name, context->listing, error, loc));
                    exit(1);
                }

                type = VAL_UNKNOWN;
                if (identifier.type == SYM_FN)
                {
                    type = VAL_FUNCTION;
                }
                else if (identifier.location.type == LOC_MEMORY)
                {
                    code_block_write(context->current_code_block, INSTRUCTION(OP_LOAD, context->rp, identifier.location.address));
                    identifier.location.type = LOC_REGISTER;
                    identifier.location.address = context->rp++;
                    symbol_map_set(context->symbols, identifier);
                }

                result = identifier.location.address;
            }
            break;

        default:
            break;
    }

    return (compile_result_t){ .location=result, .type=type, .code=NULL };
}

compile_result_t compile_tuple(ast_t *ast, compile_context_t *context)
{
    uint8_t *registers = (uint8_t *)malloc(sizeof(uint8_t) * ast->op.list.size);
    uint8_t restore_register = context->rp;

    // First, calculate the values of the tuple
    for (int i = 0; i < ast->op.list.size; i++)
    {
        compile_result_t result = compile_ast(ast->op.list.items[i], context);
        registers[i] = result.location;

        if (registers[i] == context->rp)
            context->rp++;
    }

    // Now, push them on the stack in reverse order
    for (int i = ast->op.list.size - 1; i >= 0; i--)
    {
        code_block_write(context->current_code_block, INSTRUCTION(OP_PUSH, registers[i]));
    }

    context->rp = restore_register;
    free(registers);

    // Now, set register 0 to the number of args
    code_block_write(context->current_code_block, INSTRUCTION(OP_LOADV, 0, ast->op.list.size));

    // Call tuple
    value_t builtin_name = string_create("tuple");
    memory_set(context->binary->data, context->mp, builtin_name);

    code_block_write(context->current_code_block, INSTRUCTION(OP_CALL_DYNAMIC, context->mp++));
    code_block_write(context->current_code_block, INSTRUCTION(OP_POP, context->rp));

    return (compile_result_t){ .location=context->rp, .type=VAL_TUPLE, .code=NULL };
}

compile_result_t compile_range(ast_t *ast, compile_context_t *context)
{
    uint8_t restore_register = context->rp;

    uint8_t registers[2];
    registers[0] = compile_ast(ast->op.range.begin, context).location;
    context->rp++;
    registers[1] = compile_ast(ast->op.range.end, context).location;
    context->rp = restore_register;

    compile_result_t result = write_out_builtin(context, "range", 2, registers);
    return result;

}

compile_result_t compile_unary(ast_t *ast, compile_context_t *context)
{
    compile_result_t right = compile_ast(ast->op.unary.operand, context);

    switch (ast->op.unary.operator.type)
    {
        case TOK_MINUS:
            code_block_write(context->current_code_block, INSTRUCTION(OP_NEGATE, context->rp, right.location));
            break;

        case TOK_BANG:
            code_block_write(context->current_code_block, INSTRUCTION(OP_NOT, context->rp, right.location));
            break;

        case TOK_RETURN:
            code_block_write(context->current_code_block, INSTRUCTION(OP_RETURN, right.location));
            break;

        default:
            ;
    }

    return (compile_result_t){ .location=context->rp, .type=right.type, .code=NULL };
}

compile_result_t compile_binary_comparison(uint8_t opcode, uint8_t base_register, uint8_t condition, uint8_t left, uint8_t right)
{
    code_block_t *block = code_block_create();

    // This is the false case
    code_block_write(block, INSTRUCTION(OP_LOAD, base_register, 0));

    // This is the comparison instruction
    code_block_write(block, INSTRUCTION(opcode, condition, left, right));

    // This is the true case
    code_block_write(block, INSTRUCTION(OP_LOAD, base_register, 1));

    return (compile_result_t){ .location=base_register, .type=VAL_BOOLEAN, .code=block };
}

compile_result_t compile_binary(ast_t *ast, compile_context_t *context)
{
    compile_result_t intermediate_result = {0, VAL_ABSENT, NULL};
    compile_result_t left = compile_ast(ast->op.binary.left, context);
    context->rp += 1;

    compile_result_t right;
    // The dot operator must be special cased
    if (ast->op.binary.operator.type != TOK_DOT)
    {
        right = compile_ast(ast->op.binary.right, context);
        context->rp -= 1;
    }

    value_type_e type = VAL_UNKNOWN;
    instruction_t instruction;
    switch (ast->op.binary.operator.type)
    {
        //-- Arithmetic
        case TOK_PLUS:
            instruction = INSTRUCTION(OP_ADD, context->rp, left.location, right.location);
            type = arithmetic_cast(left.type, right.type);
            break;

        case TOK_MINUS:
            instruction = INSTRUCTION(OP_SUBTRACT, context->rp, left.location, right.location);
            type = arithmetic_cast(left.type, right.type);
            break;

        case TOK_ASTERISK:
            instruction = INSTRUCTION(OP_MULTIPLY, context->rp, left.location, right.location);
            type = arithmetic_cast(left.type, right.type);
            break;

        case TOK_SLASH:
            instruction = INSTRUCTION(OP_DIVIDE, context->rp, left.location, right.location);
            type = VAL_FLOAT;
            break;

        case TOK_MODULO:
            if (left.type != VAL_INT || right.type != VAL_INT)
            {
                location_t loc;
                if (left.type != VAL_INT)
                    loc = ast->op.binary.left->location;
                else
                    loc = ast->op.binary.right->location;

                char *error = "Non-integer values are not yet supported by the modulo operator.";
                printf("%s", format_error(context->name, context->listing, error, loc));
                exit(1);
            }
            instruction = INSTRUCTION(OP_MODULO, context->rp, left.location, right.location);
            type = arithmetic_cast(left.type, right.type);
            break;

        //-- Logic
        case TOK_AND:
            instruction = INSTRUCTION(OP_AND, context->rp, left.location, right.location);
            type = VAL_BOOLEAN;
            break;

        case TOK_OR:
            instruction = INSTRUCTION(OP_OR, context->rp, left.location, right.location);
            type = VAL_BOOLEAN;
            break;

        case TOK_EQUAL_EQUAL:
            intermediate_result = compile_binary_comparison(OP_EQUAL, context->rp + 2, 1, left.location, right.location);
            instruction = INSTRUCTION(OP_MOVE, context->rp, intermediate_result.location);
            type = VAL_BOOLEAN;
            break;

        case TOK_BANG_EQUAL:
            intermediate_result = compile_binary_comparison(OP_EQUAL, context->rp + 2, 0, left.location, right.location);
            instruction = INSTRUCTION(OP_MOVE, context->rp, intermediate_result.location);
            type = VAL_BOOLEAN;
            break;

        case TOK_LESS:
            intermediate_result = compile_binary_comparison(OP_LESSTHAN, context->rp + 2, 1, left.location, right.location);
            instruction = INSTRUCTION(OP_MOVE, context->rp, intermediate_result.location);
            type = VAL_BOOLEAN;
            break;

        case TOK_LESS_EQUAL:
            intermediate_result = compile_binary_comparison(OP_LESSTHAN, context->rp + 2, 1, left.location, right.location);
            code_block_merge(context->current_code_block, intermediate_result.code);
            code_block_free(intermediate_result.code);
            intermediate_result = compile_binary_comparison(OP_EQUAL, context->rp + 3, 1, left.location, right.location);
            instruction = INSTRUCTION(OP_OR, context->rp, context->rp + 2, context->rp + 3);
            type = VAL_BOOLEAN;
            break;

        case TOK_GREATER:
            intermediate_result = compile_binary_comparison(OP_LESSTHAN, context->rp + 2, 0, left.location, right.location);
            instruction = INSTRUCTION(OP_MOVE, context->rp, intermediate_result.location);
            type = VAL_BOOLEAN;
            break;

        case TOK_GREATER_EQUAL:
            intermediate_result = compile_binary_comparison(OP_LESSTHAN, context->rp + 2, 0, left.location, right.location);
            code_block_merge(context->current_code_block, intermediate_result.code);
            code_block_free(intermediate_result.code);
            intermediate_result = compile_binary_comparison(OP_EQUAL, context->rp + 3, 1, left.location, right.location);
            instruction = INSTRUCTION(OP_OR, context->rp, context->rp + 2, context->rp + 3);
            type = VAL_BOOLEAN;
            break;

        case TOK_DOT:
        {
            // Currently, the left side must be a literal
            assert(ast->op.binary.left->type == AST_LITERAL);

            // TODO: Support more than a single level
            assert(ast->op.binary.right->type == AST_FUNCTION_CALL);

            symbol_t previous_context = context->member_context;

            context->member_context = symbol_map_get(context->symbols, ast->op.binary.left->op.literal.value);

            right = compile_ast(ast->op.binary.right, context);
            type = right.type;

            context->member_context = previous_context;

            break;
        }

        default:
            ;
    }

    if (intermediate_result.code != NULL)
    {
        code_block_merge(context->current_code_block, intermediate_result.code);
        code_block_free(intermediate_result.code);
    }

    code_block_write(context->current_code_block, instruction);
    return (compile_result_t){ .location=context->rp, .type=type, .code=NULL };
}

compile_result_t compile_assign(ast_t *ast, compile_context_t *context)
{
    // First, check our symbol to make sure it's been defined
    symbol_t symbol = symbol_map_get(context->symbols, ast->op.assign.name);

    // Error handling
    if (symbol.location.type == LOC_UNDEF)
    {
        char *error;
        location_t loc = {ast->location.start, ast->location.end};
        asprintf(&error, "Use of undeclared identifier \"%s\"", ast->op.assign.name);
        printf("%s", format_error(context->name, context->listing, error, loc));
        exit(1);
    }

    if (symbol.type == SYM_CONSTANT)
    {
        char *error;
        location_t loc = {ast->location.start, ast->location.end};
        asprintf(&error, "Cannot assign to constant \"%s\", value is immutable", ast->op.assign.name);
        printf("%s", format_error(context->name, context->listing, error, loc));
        exit(1);
    }

    compile_result_t rvalue = compile_ast(ast->op.assign.value, context);

    switch (rvalue.type)
    {
        case VAL_FUNCTION:
            symbol.location.address = rvalue.location;
            symbol.location.type = LOC_MEMORY;
            symbol_map_set(context->symbols, symbol);
            break;

        default:
            code_block_write(context->current_code_block, INSTRUCTION(OP_MOVE, symbol.location.address, rvalue.location));
            break;
    }

    return (compile_result_t){ .location=symbol.location.address, .type=rvalue.type, NULL};
}

compile_result_t compile_declare(ast_t *ast, compile_context_t *context)
{
    value_type_e type = VAL_ABSENT;
    symbol_t symbol = (symbol_t){ .location={ .address=0, .type=LOC_NONE }, .name=ast->op.declare.name };
    symbol.type = (ast->op.declare.var_type.type == TOK_VAR) ? SYM_VAR : SYM_CONSTANT;

    if (ast->op.declare.initial_value != NULL)
    {
        compile_result_t initial_value = compile_ast(ast->op.declare.initial_value, context);
        type = initial_value.type;

        symbol.location.type = LOC_REGISTER;
        if (type == VAL_FUNCTION)
        {
            symbol.location.type = LOC_MEMORY;
        }

        symbol.location.address = initial_value.location;
        context->rp += 1;
    }

    symbol_map_set(context->symbols, symbol);
    return (compile_result_t){ .location=symbol.location.address, .type=type, .code=NULL};
}

compile_result_t compile_fn_declaration(ast_t *ast, compile_context_t *context)
{
    // Create our inner scope for this function
    symbol_map_t *inner_scope = symbol_map_create();
    inner_scope->parent = context->symbols;
    context->symbols = inner_scope;

    // Create a new code block for our function
    uint64_t previous_code_pointer = context->cp;
    code_block_t *previous_code_block = context->current_code_block;
    code_block_t *fn_block = code_block_create();
    code_collection_add_block(context->binary->code, fn_block);
    context->cp = context->binary->code->size - 1;
    context->current_code_block = fn_block;

    uint8_t restore_register = context->rp;

    ast_t *args = ast->op.fn.args;
    value_t fn_def = function_def_create(
                ast->op.fn.name,
                (address_t){ .region=context->cp, .offset=0 },
                (args == NULL) ? 0 : args->op.list.size,
                NULL,
                context->rp
            );
    memory_set(context->binary->data, context->mp, fn_def);

    symbol_t symbol;
    symbol.name = ast->op.fn.name;
    symbol.type = SYM_FN;
    symbol.location.type = LOC_MEMORY;
    symbol.location.address = context->mp++;

    symbol_map_set(context->symbols, symbol);

    if (args != NULL)
    {
        for (int i = args->op.list.size - 1; i >= 0; i--)
        {
            symbol_t arg;
            arg.location.type = LOC_REGISTER;
            arg.location.address = context->rp + i;
            arg.name = args->op.list.items[i]->op.literal.value;
            arg.type = SYM_VAR;
            symbol_map_set(context->symbols, arg);
        }
        context->rp += args->op.list.size;
    }

    // Construct locals to keep track of
    uint8_t *locals = (uint8_t *)malloc(context->rp - restore_register + 1);
    for (int i = restore_register; i < context->rp + 1; i++)
    {
        locals[i - restore_register] = i;
    }
    locals[context->rp - restore_register + 1] = 0;

    function_t *fn_prototype = (function_t *)fn_def.contents.object;
    fn_prototype->locals = locals;

    compile_result_t fn_result = compile_ast(ast->op.fn.body, context);
    code_block_free(fn_result.code);

    // If return was implicit, add it in now
    size_t last = context->current_code_block->size - 1;
    if (context->current_code_block->code[last].opcode != OP_RETURN)
        code_block_write(context->current_code_block, INSTRUCTION(OP_RETURN, fn_result.location));

    // Reset state
    symbol_map_t *map = context->symbols;
    context->symbols = context->symbols->parent;
    symbol_map_destroy(map);
    context->cp = previous_code_pointer;
    context->current_code_block = previous_code_block;

    symbol_map_set(context->symbols, symbol);
    context->rp = restore_register;

    // If the function is external, put it in our binary symbol map
    if (ast->op.fn.exported)
        symbol_map_set(context->binary->symbols, symbol);

    return (compile_result_t){ .location=symbol.location.address, .type=VAL_FUNCTION, .code=NULL };
}

compile_result_t compile_fn_call_builtin(ast_t *ast, compile_context_t *context)
{
    ast_t *args = ast->op.call.args;
    uint8_t *arg_registers;
    uint8_t number_of_args = 0;
    if (args != NULL)
    {
        arg_registers = (uint8_t *)malloc(sizeof(uint8_t) * args->op.list.size);
        number_of_args = args->op.list.size;

        // First, calculate the values of our arguments
        for (int i = 0; i < args->op.list.size; i++)
        {
            compile_result_t arg = compile_ast(args->op.list.items[i], context);
            arg_registers[i] = arg.location;

            if (arg_registers[i] == context->rp)
                context->rp++;
        }
    }

    compile_result_t result = write_out_builtin(context, ast->op.fn.name, number_of_args, arg_registers);

    free(arg_registers);

    return result;
}

compile_result_t compile_fn_call_native(ast_t *ast, compile_context_t *context)
{
    symbol_t fn_symbol = symbol_map_get(context->symbols, ast->op.call.name);
    ast_t *args = ast->op.call.args;

    // Native function calls must exist in memory
    assert(fn_symbol.location.type == LOC_MEMORY);

    function_t *function = (function_t *)memory_get(context->binary->data, fn_symbol.location.address).contents.object;

    // Now, iterate through the function args and save any locals that conflict
    for (uint8_t i = 0; i < function->nargs; i++)
    {

        if (function->locals == NULL)
        {
            if (function->low_reg + i >= context->rp)
                continue;

            code_block_write(context->current_code_block, INSTRUCTION(OP_PUSH, function->low_reg + i));
        }
        else if (function->locals[i] < context->rp)
            code_block_write(context->current_code_block, INSTRUCTION(OP_PUSH, function->locals[i]));
    }


    uint8_t restore_register = context->rp;
    if (args != NULL)
    {
        if (args->op.list.size != function->nargs)
        {
            char *error;
            location_t loc = {ast->location.start, ast->location.end};
            asprintf(&error, "Function \"%s\" expected %d arguments, but was passed %ld.",
                     function->name,
                     function->nargs,
                     args->op.list.size
            );
            printf("%s", format_error(context->name, context->listing, error, loc));
            exit(1);
        }

        for (int i = 0; i < args->op.list.size; i++)
        {
            compile_result_t arg = compile_ast(args->op.list.items[i], context);

            // If we weren't serendipitous, move things around
            if (arg.location != function->locals[i])
                code_block_write(context->current_code_block, INSTRUCTION(OP_MOVE, function->locals[i], arg.location));

            context->rp += 1;
        }
    }
    else if (function->nargs > 0)
    {
        char *error;
        location_t loc = {ast->location.start, ast->location.end};
        asprintf(&error, "Function \"%s\" expected %d arguments, but was passed none.",
                 function->name,
                 function->nargs
        );
        printf("%s", format_error(context->name, context->listing, error, loc));
        exit(1);
    }

    // Call the function
    code_block_write(context->current_code_block, INSTRUCTION(OP_CALL, fn_symbol.location.address));

    // Restore RP
    context->rp = restore_register;

    // Pop the return value
    code_block_write(context->current_code_block, INSTRUCTION(OP_POP, context->rp));

    // Pop our locals back into registers
    for (int i = function->nargs - 1; i >= 0; --i)
    {
        if (function->low_reg + i < context->rp)
            code_block_write(context->current_code_block, INSTRUCTION(OP_POP, function->low_reg + i));
    }

    return (compile_result_t){ .location=context->rp, .type=VAL_UNKNOWN, .code=NULL };
}

compile_result_t compile_fn_call(ast_t *ast, compile_context_t *context)
{
    symbol_t fn_symbol = symbol_map_get(context->symbols, ast->op.call.name);

    switch (fn_symbol.location.type)
    {
        case LOC_MEMORY:
            return compile_fn_call_native(ast, context);

        case LOC_BUILTIN:
        case LOC_UNDEF:
            return compile_fn_call_builtin(ast, context);

        default:
            ;
    }

    return (compile_result_t){};
}

compile_result_t compile_if_statement(ast_t *ast, compile_context_t *context)
{
    uint8_t restore_register = context->rp;

    // Compile the parts of our if statement
    compile_result_t if_condition = compile_ast(ast->op.if_stmt.condition, context);
    compile_result_t if_body = compile_statement_list(ast->op.if_stmt.body, context);

    // Increment rp if necessary
    if (if_body.location == context->rp)
        context->rp += 1;

    // How far ahead to jump if we evaluate to false
    code_block_write(context->current_code_block, INSTRUCTION(OP_LOADV, context->rp++, if_body.code->size + 1));

    // Test the condition
    code_block_write(context->current_code_block, INSTRUCTION(OP_LOAD, context->rp, 1));
    code_block_write(context->current_code_block, INSTRUCTION(OP_EQUAL, 0, context->rp, if_condition.location));

    // Jump if false
    code_block_write(context->current_code_block, INSTRUCTION(OP_JMP, context->rp - 1));

    context->rp = restore_register;

    code_block_merge(context->current_code_block, if_body.code);
    code_block_free(if_body.code);

    return (compile_result_t){ .location=context->rp, .type=VAL_UNKNOWN, .code=NULL };
}

compile_result_t compile_for_statement(ast_t *ast, compile_context_t *context)
{
    // Compile our collection
    compile_result_t collection = compile_ast(ast->op.for_stmt.iterable, context);

    // Next, make an iterator over the collection
    compile_result_t iter_result = write_out_builtin(context, "iter", 1, &collection.location);

    // Need to keep track of our iterator
    context->rp += 1;

    // Now, define a new symbol map for the for loop
    symbol_map_t *for_map = symbol_map_create();
    for_map->parent = context->symbols;
    context->symbols = for_map;

    // This will point to our local variable. We'll need this even if it's not
    // defined by the user.
    uint8_t var = context->rp++;
    uint8_t nil = context->rp++;

    code_block_write(context->current_code_block, INSTRUCTION(OP_NIL, nil));

    // If a local variable was defined, set it in the synbol map
    if (ast->op.for_stmt.var != NULL)
    {
        symbol_t symbol = { .type=SYM_VAR, .name=ast->op.for_stmt.var, .location={ .type=LOC_REGISTER, .address=var } };
        symbol_map_set(context->symbols, symbol);
    }

    // Compile our body
    compile_result_t for_body = compile_statement_list(ast->op.for_stmt.body, context);

    // Begin for-loop
    code_block_write(context->current_code_block, INSTRUCTION(OP_DEREF, var, iter_result.location, 1));

    // Exit jump
    code_block_write(context->current_code_block, INSTRUCTION(OP_LOADV, context->rp, for_body.code->size + 3));
    code_block_write(context->current_code_block, INSTRUCTION(OP_EQUAL, 1, var, nil));
    code_block_write(context->current_code_block, INSTRUCTION(OP_JMP, context->rp));

    code_block_merge(context->current_code_block, for_body.code);

    // Jump to the start of the loop
    code_block_write(context->current_code_block, INSTRUCTION(OP_LOADV, context->rp, (for_body.code->size + 5) * -1));
    code_block_write(context->current_code_block, INSTRUCTION(OP_JMP, context->rp));
    code_block_free(for_body.code);

    context->rp -= 2;

    // Reset symbol map
    context->symbols = context->symbols->parent;
    symbol_map_destroy(for_map);

    return (compile_result_t){ .location=context->rp, .type=VAL_UNKNOWN, .code=NULL };
}

compile_result_t compile_module(ast_t *ast, compile_context_t *context)
{
    value_t module_name = string_create(ast->op.module.name);

    memory_set(context->binary->data, context->mp, module_name);

    const char *symbol_name = symbol_name_for_module_path(ast->op.module.name);
    symbol_t module = (symbol_t){ .location={ .address=context->mp, .type=LOC_MEMORY }, .name=symbol_name };
    symbol_map_set(context->symbols, module);

    code_block_write(context->current_code_block, INSTRUCTION(OP_IMPORT, context->mp++));

    return (compile_result_t){ .location=context->rp, .type=VAL_MODULE, .code=NULL };
}

compile_result_t compile_ast(ast_t *ast, compile_context_t *context)
{
    compile_result_t result;
    switch (ast->type)
    {
        case AST_STMT_LIST:
            result = compile_statement_list(ast, context);
            // Here we merge the result into the current code block
            code_block_merge(context->current_code_block, result.code);
            break;

        case AST_LITERAL:
            result = compile_literal(ast, context);
            break;

        case AST_UNARY:
            result = compile_unary(ast, context);
            break;

        case AST_BINARY:
            result = compile_binary(ast, context);
            break;

        case AST_DECLARE:
            result = compile_declare(ast, context);
            break;

        case AST_ASSIGN:
            result = compile_assign(ast, context);
            break;

        case AST_GROUP:
            result = compile_ast(ast->op.group, context);
            break;

        case AST_FUNCTION_DECL:
            result = compile_fn_declaration(ast, context);
            break;

        case AST_FUNCTION_CALL:
            result = compile_fn_call(ast, context);
            break;

        case AST_TUPLE:
            result = compile_tuple(ast, context);
            break;

        case AST_RANGE:
            result = compile_range(ast, context);
            break;

        case AST_IF_STMT:
            result = compile_if_statement(ast, context);
            break;

        case AST_FOR_STMT:
            result = compile_for_statement(ast, context);
            break;

        case AST_MODULE:
            result = compile_module(ast, context);
            break;

        default:
            ;
    }
    return result;
}

binary_t *compile(const char *name, const char *listing, ast_t *ast)
{
    compile_context_t *context = context_create(name, listing);
    compile_ast(ast, context);
    binary_t *binary = context->binary;
    context_destroy(context);
    return binary;
}
