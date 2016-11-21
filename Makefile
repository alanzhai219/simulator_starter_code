# 18-447 RISC-V 32-bit Simulator Makefile
#
# ECE 18-447
# Carnegie Mellon University
#
# This Makefile compiles the simulator and assembles test into RISC-V code for
# the simulator.
#
# Authors:
# 	- 2016: Brandon Perez

################################################################################
#                           DO NOT MODIFY THIS FILE!                           #
#                  You should only add files or change sim.c!                  #
################################################################################

################################################################################
# User Controlled Parameters
################################################################################

# The user can specify the test they want to assemble. Defaults to addi.s
DEFAULT_TEST = 447inputs/addi.s
TEST ?= $(DEFAULT_TEST)

################################################################################
# General Targets and Variables
################################################################################

.PHONY: all default clean veryclean

# By default, compile the simulator
all default: sim

# Cleanup the intermediate files generated by compiling the simulator
clean: sim-clean

# Cleanup the intermediate files generated by assemling test programs
veryclean: clean assemble-veryclean

################################################################################
# Assemble Test Programs
################################################################################

# These targets don't correspond to actual files
.PHONY: assemble assemble-veryclean

# Prevent make from automatically deleting intermediate files generated.
# Specifically, prevent the deletion of the binary files.
.SECONDARY:

# The name of the entrypoint for assembly tests, which matches the typical main
RISCV_ENTRY_POINT = main

# The compiler for assembly files, along with its flags
RISCV_CC = riscv64-unknown-elf-gcc
RISCV_CFLAGS = -static -nostdlib -nostartfiles
RISCV_AS_LDFLAGS = -Wl,-e$(RISCV_ENTRY_POINT)

# The objcopy utility for assembly files, along with its flags
RISCV_OBJCOPY = riscv64-unknown-elf-objcopy
RISCV_OBJCOPY_FLAGS = -O binary

# The compiler for hex files, which convert copied binary to ASCII hex files,
# where there is one word per line.
HEX_CC = hexdump
HEX_CFLAGS = -e '1/4 "%08X" "\n"'

# The runtime environment directory, which has the startup file for C programs
447_RUNTIME_DIR = 447runtime
RISCV_STARTUP_FILE = $(447_RUNTIME_DIR)/crt0.S

# The sections of the ELF executable that we parse out, and file extensions
SECTIONS = text data ktext kdata
ELF_EXTENSION = elf
BINARY_EXTENSION = bin
HEX_EXTENSION = hex

# The name of the test, and the hex and ELF files generated for the given test
TEST_NAME = $(basename $(TEST))
TEST_EXECUTABLE = $(addsuffix .$(ELF_EXTENSION), $(TEST_NAME))
TEST_SECTIONS = $(addprefix $(TEST_NAME).,$(SECTIONS))
TEST_SECTIONS_HEX = $(addsuffix .$(HEX_EXTENSION),$(TEST_SECTIONS))

# Assemble the program specified by the user on the command line
assemble: $(TEST_SECTIONS_HEX)

# Convert a binary file for program of the ELF file to an ASCII hex
%.$(HEX_EXTENSION): %.$(BINARY_EXTENSION)
	$(HEX_CC) $(HEX_CFLAGS) $^ > $@

# Extract the given section from the program ELF file, generating a binary
$(TEST_NAME).%.$(BINARY_EXTENSION): $(TEST_EXECUTABLE)
	$(RISCV_OBJCOPY) $(RISCV_OBJCOPY_FLAGS) -j .$* $^ $@

# Compile the assembly test program with a *.s extension to create an ELF file
%.$(ELF_EXTENSION): %.s
	$(RISCV_CC) $(RISCV_CFLAGS) $(RISCV_AS_LDFLAGS) $^ -o $@

# Compile the assembly test program with a *.S extension to create an ELF file
%.$(ELF_EXTENSION): %.S
	$(RISCV_CC) $(RISCV_CFLAGS) $(RISCV_AS_LDFLAGS) $^ -o $@

# Compile the C test program with the startup file to create an ELF file
%.$(ELF_EXTENSION): $(RISCV_STARTUP_FILE) %.c
	$(RISCV_CC) $(RISCV_CFLAGS) $^ -o $@

# Clean up all the hex files in project directories
assemble-veryclean:
	rm -f $$(find -name '*.$(HEX_EXTENSION)' -o -name '*.$(BINARY_EXTENSION)' \
			-o -name '*.$(ELF_EXTENSION)')

################################################################################
# Compile the Simulator
################################################################################

# These targets don't correspond to actual files
.PHONY: sim sim-clean

# The compiler for the simulator, along with its flags
SIM_CC = gcc
SIM_CFLAGS = -Wall -Wextra -std=gnu99 -pedantic -g

# The directory for the simulator source files, and all of the files for it
SIM_SRC_DIR = src
SIM = $(shell find $(SIM_SRC_DIR) -type f -name '*.c' -o -name '*.h')

# The name of the exeuctable generated by compiling the simulator
SIM_EXECUTABLE = riscv-sim

# User-facing target to compile the simulator into an executable
sim: $(SIM_EXECUTABLE)

# Compile the simulator into an executable
$(SIM_EXECUTABLE): $(SIM)
	$(SIM_CC) $(SIM_CFLAGS) $(filter %.c,$^) -o $@

# Cleanup any intermediate files generated by compiling the simulator
sim-clean:
	rm -f $(SIM_EXECUTABLE)
