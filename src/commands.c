/**
 * commands.c
 *
 * RISC-V 32-bit Instruction Level Simulator
 *
 * ECE 18-447
 * Carnegie Mellon University
 *
 * This file contains the implementation for the shell commands.
 *
 * The commands are how the user interacts with the simulator from the shell.
 * The commands are pretty basic, such as stepping the program, displaying
 * registers, etc.
 *
 * Authors:
 *  - 2016: Brandon Perez
 **/

/*----------------------------------------------------------------------------*
 *                          DO NOT MODIFY THIS FILE!                          *
 *                 You should only add files or change sim.c!                 *
 *----------------------------------------------------------------------------*/

#include <stdlib.h>             // Malloc and related functions
#include <stdio.h>              // Printf and related functions
#include <stdint.h>             // Fixed-size integral types

#include <limits.h>             // Limits for integer types
#include <assert.h>             // Assert macro
#include <string.h>             // String manipulation functions and memset
#include <errno.h>              // Error codes and perror

#include "sim.h"                // Definition of cpu_state_t
#include "commands.h"           // This file's interface
#include "parse.h"              // Parsing utilities

/*----------------------------------------------------------------------------
 * Internal Definitions
 *----------------------------------------------------------------------------*/

// Macro to get the length of a statically allocated array
#define array_len(x)            (sizeof(x) / sizeof((x)[0]))

/*----------------------------------------------------------------------------
 * Step and Go Commands
 *----------------------------------------------------------------------------*/

// The maximum number of arguments that can be specified to the step command
#define STEP_MAX_NUM_ARGS       1

// The expected number of arguments for the go command
#define GO_NUM_ARGS             0

/**
 * run_simulator
 *
 * Run the simulator for a single cycle, incrementing the instruction count.
 **/
static void run_simulator(cpu_state_t *cpu_state)
{
    process_instruction(cpu_state);
    cpu_state->instr_count += 1;
    return;
}

/**
 * command_step
 *
 * Runs a the simulator for a specified number of cycles or until the processor
 * is halted. The user can optionally specify the number of cycles, otherwise
 * the default is one.
 **/
void command_step(cpu_state_t *cpu_state, const char *args[], int num_args)
{
    // Check that the appropiate number of arguments was specified
    if (num_args > STEP_MAX_NUM_ARGS) {
        fprintf(stderr, "Error: Too many arguments specified to 'step' "
                "command.\n");
        return;
    }

    // If a number of cycles was specified, then attempt to parse it
    int num_cycles = 1;
    if (num_args != 0 && parse_int(args[0], &num_cycles) < 0) {
        fprintf(stderr, "Error: Unable to parse '%s' as an int.\n", args[0]);
        return;
    }

    // If the processor is halted, then we don't do anything.
    if (cpu_state->halted) {
        fprintf(stdout, "Processor is halted, cannot run the simulator.\n");
        return;
    }

    /* Run the simulator for the specified number of cycles, or until the
     * processor is halted. */
    for (int i = 0; i < num_cycles && !cpu_state->halted; i++)
    {
        run_simulator(cpu_state);
    }

    return;
}

/**
 * command_go
 *
 * Runs the simulator until program completion or an exception is encountered.
 **/
void command_go(cpu_state_t *cpu_state, const char *args[], int num_args)
{
    // Silence unused variable warnings from the compiler
    (void)args;

    // Check that the appropiate number of arguments was specified
    if (num_args != GO_NUM_ARGS) {
        fprintf(stderr, "Error: Improper number of arguments specified to "
                "'go' command.\n");
        return;
    }

    // If the processor is halted, then we don't do anything.
    if (cpu_state->halted) {
        fprintf(stdout, "Processor is halted, cannot run the simulator.\n");
        return;
    }

    // Run the simulator until the processor is halted
    while (!cpu_state->halted)
    {
        run_simulator(cpu_state);
    }

    return;
}

/*----------------------------------------------------------------------------
 * Reg and Rdump Commands
 *----------------------------------------------------------------------------*/

// The minimum and maximum expected number of arguments for the reg command
#define REG_MIN_NUM_ARGS        1
#define REG_MAX_NUM_ARGS        2

// The maximum expected number of arguments for the rdump command
#define RDUMP_MAX_NUM_ARGS      1

// The maximum length of an ISA and ABI alias name for a register
#define REG_ISA_MAX_LEN         3
#define REG_ABI_MAX_LEN         5

// The maximum number of decimal digits for a 32-bit integer value
#define INT32_MAX_DEC_DIGITS    10

// Structure representing the naming information about a register
typedef struct register_name {
    const char *isa_name;       // The ISA name for a register (x0..x31)
    const char *abi_name;       // The ABI name for a register (sp, t0, etc.)
} register_name_t;

// An array of all the naming information for a register, and its ABI aliases
static const register_name_t RISCV_REGISTER_NAMES[RISCV_NUM_REGS] = {
    { .isa_name = "x0", .abi_name = "zero", },
    { .isa_name = "x1", .abi_name = "ra", },
    { .isa_name = "x2", .abi_name = "sp", },
    { .isa_name = "x3", .abi_name = "gp", },
    { .isa_name = "x4", .abi_name = "tp", },
    { .isa_name = "x5", .abi_name = "t0", },
    { .isa_name = "x6", .abi_name = "t1", },
    { .isa_name = "x7", .abi_name = "t2", },
    { .isa_name = "x8", .abi_name = "s0/fp", },
    { .isa_name = "x9", .abi_name = "s1", },
    { .isa_name = "x10", .abi_name = "a0", },
    { .isa_name = "x11", .abi_name = "a1", },
    { .isa_name = "x12", .abi_name = "a2", },
    { .isa_name = "x13", .abi_name = "a3", },
    { .isa_name = "x14", .abi_name = "a4", },
    { .isa_name = "x15", .abi_name = "a5", },
    { .isa_name = "x16", .abi_name = "a6", },
    { .isa_name = "x17", .abi_name = "a7", },
    { .isa_name = "x18", .abi_name = "s2", },
    { .isa_name = "x19", .abi_name = "s3", },
    { .isa_name = "x20", .abi_name = "s4", },
    { .isa_name = "x21", .abi_name = "s5", },
    { .isa_name = "x22", .abi_name = "s6", },
    { .isa_name = "x23", .abi_name = "s7", },
    { .isa_name = "x24", .abi_name = "s8", },
    { .isa_name = "x25", .abi_name = "s9", },
    { .isa_name = "x26", .abi_name = "s10", },
    { .isa_name = "x27", .abi_name = "s11", },
    { .isa_name = "x28", .abi_name = "t3", },
    { .isa_name = "x29", .abi_name = "t4", },
    { .isa_name = "x30", .abi_name = "t5", },
    { .isa_name = "x31", .abi_name = "t6", },
};

/**
 * find_register
 *
 * Tries to find the register with a matching ISA name or ABI alias from the
 * available registers. Returns a register number [0..31] on success, or a
 * negative number on failure.
 **/
static int find_register(const char *reg_name)
{
    // Iterate over each register, and try to find a match to the name
    for (int i = 0; i < (int)array_len(RISCV_REGISTER_NAMES); i++)
    {
        const register_name_t *reg_info = &RISCV_REGISTER_NAMES[i];
        if (strcmp(reg_name, reg_info->isa_name) == 0) {
            return i;
        } else if (strcmp(reg_name, reg_info->abi_name) == 0) {
            return i;
        }
    }

    // No matching registers were found
    return -ENOENT;
}

/**
 * print_register
 *
 * Prints out the information for a given register on one line.
 **/
static void print_register(cpu_state_t *cpu_state, int reg_num, FILE *file)
{
    assert(0 <= reg_num && reg_num < (int)array_len(RISCV_REGISTER_NAMES));

    // Format the ABI alias name for the register, surrounded with parenthesis
    int abi_name_max_len = REG_ABI_MAX_LEN + 2;
    char abi_name[abi_name_max_len+1];
    const register_name_t *reg_name = &RISCV_REGISTER_NAMES[reg_num];
    size_t written = snprintf(abi_name, sizeof(abi_name), "(%s)",
            reg_name->abi_name);
    assert(written < sizeof(abi_name));


    // Format the unsigned view of the register's surrounded with parenthesis
    uint32_t reg_value = cpu_state->regs[reg_num];
    int reg_value_max_len = INT32_MAX_DEC_DIGITS + 2;
    char reg_uint_value[reg_value_max_len+1];
    written = snprintf(reg_uint_value, sizeof(reg_uint_value), "(%u)",
            reg_value);
    assert(written < sizeof(reg_uint_value));

    // Format the signed view of the register's surrounded with parenthesis
    char reg_int_value[reg_value_max_len+1];
    written = snprintf(reg_int_value, sizeof(reg_int_value), "(%d)",
            (int32_t)reg_value);
    assert(written < sizeof(reg_int_value));

    // Print out the register names and its values
    fprintf(file, "%-*s %-*s = 0x%08x %-*s %-*s\n", REG_ISA_MAX_LEN,
            reg_name->isa_name, abi_name_max_len, abi_name, reg_value,
            reg_value_max_len, reg_int_value, reg_value_max_len,
            reg_uint_value);
    return;
}

/**
 * command_reg
 *
 * Display the value of the specified register to the user. The user can
 * optionally specify a value to update the register's value instead.
 **/
void command_reg(cpu_state_t *cpu_state, const char *args[], int num_args)
{
    assert(REG_MAX_NUM_ARGS - REG_MIN_NUM_ARGS == 1);
    assert(array_len(cpu_state->regs) == array_len(RISCV_REGISTER_NAMES));

    // Check that the appropriate number of arguments was specified
    if (num_args < REG_MIN_NUM_ARGS) {
        fprintf(stderr, "Error: reg: Too few arguments specified.\n");
        return;
    } else if (num_args > REG_MAX_NUM_ARGS) {
        fprintf(stderr, "Error: reg: Too many arguments specified.\n");
        return;
    }

    /* First, try to parse the register argument as an integer, then try to
     * parse it as string for one of its names. */
    const char *reg_string = args[0];
    int reg_num = -ENOENT;
    if (parse_int(reg_string, &reg_num) < 0) {
        reg_num = find_register(reg_string);
    }

    // If we couldn't parse the given register, or it is out of range, stop
    if (reg_num < 0 || reg_num >= (int)array_len(cpu_state->regs)) {
        fprintf(stderr, "Error: reg: Invalid register '%s' specified.\n",
                reg_string);
        return;
    }

    // If the user didn't specify a value, then we simply print the register out
    if (num_args == REG_MIN_NUM_ARGS) {
        print_register(cpu_state, reg_num, stdout);
        return;
    }

    // Otherwise, parse the second argument as a 32-bit integer
    const char *reg_value_string = args[1];
    int32_t reg_value;
    if (parse_int32(reg_value_string, &reg_value) < 0) {
        fprintf(stderr, "Error: reg: Unable to parse '%s' as a 32-bit "
                "integer.\n", reg_value_string);
        return;
    }

    // Update the register with the new value
    cpu_state->regs[reg_num] = (uint32_t)reg_value;
    return;
}

/**
 * comand_rdump
 *
 * Displays the value of all registers in the system, along with the number of
 * instructions executed so far. The user can optionally specify a file to dump
 * the values to.
 **/
void command_rdump(cpu_state_t *cpu_state, const char *args[], int num_args)
{
    // Check that the appropriate number of arguments was specified
    if (num_args > RDUMP_MAX_NUM_ARGS) {
        fprintf(stderr, "Error: rdump: Too many arguments specified.\n");
        return;
    }

    /* If specified, dump the register values to the given file. Otherwise,
     * print the values to stdout. */
    FILE *dump_file = stdout;
    if (num_args == RDUMP_MAX_NUM_ARGS) {
        const char *dump_filename = args[0];
        dump_file = fopen(dump_filename, "w");
        if (dump_file == NULL) {
            fprintf(stderr, "Error: rdump: %s: Unable to open file: %s.\n",
                    dump_filename, strerror(errno));
            return;
        }
    }

    // Print out the header for the register dump
    fprintf(dump_file, "Current CPU State and Register Values:\n");
    fprintf(dump_file, "--------------------------------------\n");
    fprintf(dump_file, "%-20s = %d\n", "Instruction Count",
            cpu_state->instr_count);
    fprintf(dump_file, "%-20s = 0x%08x\n", "Program Counter (PC)",
            cpu_state->pc);
    fprintf(dump_file, "\nRegister Values:\n");
    fprintf(dump_file, "--------------------------------------\n");

    // Print out all of the general purpose register values
    for (int i = 0; i < (int)array_len(cpu_state->regs); i++)
    {
        print_register(cpu_state, i, dump_file);
    }

    // Close the dump file if was specified by the user (not stdout)
    if (dump_file != stdout) {
        fclose(dump_file);
    }
    return;
}

/*----------------------------------------------------------------------------
 * Memory and Mdump Commands
 *----------------------------------------------------------------------------*/

/**
 * command_memory
 *
 * Displays the value of the specified memory address to the user. The user can
 * optionally specify a value to update the memory locations value instead.
 **/
void command_memory(cpu_state_t *cpu_state, const char *args[], int num_args)
{
    // Silence the compiler
    (void)cpu_state;
    (void)args;
    (void)num_args;

    return;
}

/**
 * command_mdump
 *
 * Displays the values of a range of memory locations in the system. The user
 * can optionally specify a file to dump the memory values to.
 **/
void command_mdump(cpu_state_t *cpu_state, const char *args[], int num_args)
{
    // Silence the compiler
    (void)cpu_state;
    (void)args;
    (void)num_args;

    return;
}

/*----------------------------------------------------------------------------
 * Restart and Load Commands
 *----------------------------------------------------------------------------*/

/**
 * command_restart
 *
 * Resets the processor and restarts the currently loaded program from its first
 * instruction.
 **/
void command_restart(cpu_state_t *cpu_state, const char *args[], int num_args)
{
    // Silence the compiler
    (void)cpu_state;
    (void)args;
    (void)num_args;

    return;
}

/**
 * command_load
 *
 * Resets the processor and loads a new program into the processor, replacing
 * the currently executing program. The execution starts from the beginning of
 * the loaded program.
 **/
void command_load(cpu_state_t *cpu_state, const char *args[], int num_args)
{
    // Silence the compiler
    (void)cpu_state;
    (void)args;
    (void)num_args;

    return;
}

/*----------------------------------------------------------------------------
 * Help and Quit Commands
 *----------------------------------------------------------------------------*/

/**
 * command_quit
 *
 * Quits the simulator.
 **/
void command_quit(cpu_state_t *cpu_state, const char *args[], int num_args)
{
    // Silence the compiler
    (void)cpu_state;
    (void)args;
    (void)num_args;

    return;
}

/**
 * command_help
 *
 * Displays a help message to the user, explaining the commands in for the
 * simulator and how to use them.
 **/
void command_help(cpu_state_t *cpu_state, const char *args[], int num_args)
{
    // Silence the compiler
    (void)cpu_state;
    (void)args;
    (void)num_args;

    return;
}
