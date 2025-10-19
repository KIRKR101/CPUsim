#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // For uint16_t

// --- Configuration Constants ---
#define MEMORY_SIZE 256             // The total size of the main memory.
#define PROGRAM_SIZE 256            // Maximum number of instructions in a program.
#define MAX_FILENAME_LENGTH 256     // Maximum length for file paths.
#define NUM_REGISTERS 8             // The number of general-purpose registers.
#define STACK_TOP (MEMORY_SIZE - 1) // The stack grows downwards from the top of memory.

// --- Core Data Structures ---
// Holds the state of the CPU's general-purpose registers.
typedef struct {
    int EAX, EBX, ECX, EDX;
    int ESI, EDI;
    int EBP, ESP;
} Registers;

// Holds the state of the CPU's flags.
typedef struct {
    int ZF; // Zero Flag
    int SF; // Sign Flag
} Flags;

// --- Global State ---
Registers registers = { 0 }; // The CPU registers.
Flags flags = { 0 }; // The CPU flags.
const char* register_names[] = { "EAX", "EBX", "ECX", "EDX", "ESI", "EDI", "EBP", "ESP" }; // Names of the registers for printing.
int memory[MEMORY_SIZE]; // The main memory.
uint16_t machine_code[PROGRAM_SIZE] = { 0 }; // Buffer for the machine code.
int program_instruction_count = 0; // The number of instructions in the loaded program.

// --- Function Prototypes ---
void dump_contents();
int  load_binary_program(const char* filename);
void run_program();
int  execute_instruction(uint16_t instruction, int pc);
int  get_register_value(int reg_code);
void set_register_value(int reg_code, int value);
void write_memory(int address, int data);
int  read_memory(int address);

// --- Main Function ---
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <binary file>\n", argv[0]);
        return 1;
    }

    if (load_binary_program(argv[1]) < 0) {
        fprintf(stderr, "[Fatal Error] Could not load binary file. Exiting.\n");
        return 1;
    }

    run_program();

    return 0;
}

int load_binary_program(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        perror("[Loader Error] Failed to open binary file");
        return -1;
    }

    // Read the binary file into the machine code buffer.
    size_t instructions_read = fread(machine_code, sizeof(uint16_t), PROGRAM_SIZE, f);
    if (instructions_read == 0 && !feof(f)) {
        fprintf(stderr, "[Loader Error] An error occurred while reading the file.\n");
        fclose(f);
        return -1;
    }
    fclose(f);

    program_instruction_count = instructions_read;
    printf("Loaded %d instructions from '%s'.\n", program_instruction_count, filename);
    return program_instruction_count;
}

void run_program() {
    int pc = 0; // The program counter starts at 0.
    memset(memory, 0, sizeof(memory)); // Clear main memory before execution.
    registers.ESP = STACK_TOP + 1; // ESP starts just above the highest memory address.
    registers.EBP = registers.ESP;

    while (pc >= 0 && pc < program_instruction_count) {
        int next_pc = execute_instruction(machine_code[pc], pc);
        pc = next_pc;
    }
}

int execute_instruction(uint16_t instruction, int pc) {
    uint16_t opcode = instruction >> 11;

    // Handle base+offset addressing for MOV instructions.
    if (opcode == 0b01111 || opcode == 0b11111) {
        uint16_t reg_dest_src = (instruction >> 8) & 0x07;
        uint16_t reg_base = (instruction >> 5) & 0x07;
        uint16_t offset = instruction & 0x1F;
        int effective_address = get_register_value(reg_base) + offset;

        if (opcode == 0b01111) { // MOV reg, [reg+off]
            set_register_value(reg_dest_src, read_memory(effective_address));
        } else { // MOV [reg+off], reg
            write_memory(effective_address, get_register_value(reg_dest_src));
        }
        return pc + 1;
    }

    // Decode the instruction's operands.
    uint16_t reg1 = (instruction >> 8) & 0x07;
    uint16_t reg2 = (instruction >> 5) & 0x07;
    uint16_t value = instruction & 0xFF;
    uint16_t addr = instruction & 0xFF;

    // Dispatch the instruction based on its opcode.
    switch (opcode) {
        // System & Memory
        case 0b00000: printf("--- HLT instruction at PC %d ---\n", pc); return -1;
        case 0b00001: set_register_value(reg1, get_register_value(reg1) * get_register_value(reg2)); break;
        case 0b00010: {
            int divisor = get_register_value(reg2);
            if (divisor == 0) {
                fprintf(stderr, "[Runtime Error] Division by zero at PC %d.\n", pc);
                return -1; // Halt on error.
            }
            set_register_value(reg1, get_register_value(reg1) / divisor);
            break;
        }
        case 0b00011: set_register_value(reg1, get_register_value(reg1) ^ get_register_value(reg2)); break;
        case 0b00100: {
            int input_val;
            printf("INPUT required for register %s: ", register_names[reg1]);
            if (scanf("%d", &input_val) != 1) {
                fprintf(stderr, "[Runtime Error] Invalid integer input.\n");
                while (getchar() != '\n'); set_register_value(reg1, 0);
            } else {
                set_register_value(reg1, input_val);
                while (getchar() != '\n');
            }
            break;
        }
        case 0b00101: printf("OUTPUT from register %s: %d\n", register_names[reg1], get_register_value(reg1)); break;
        case 0b00110: set_register_value(reg1, value); break;
        case 0b00111: set_register_value(reg1, read_memory(addr)); break;
        case 0b01000: write_memory(addr, get_register_value(reg1)); break;

        // Arithmetic
        case 0b01001: set_register_value(reg1, get_register_value(reg1) + 1); break;
        case 0b01010: set_register_value(reg1, get_register_value(reg1) - 1); break;
        case 0b10000: set_register_value(reg1, get_register_value(reg1) + get_register_value(reg2)); break;
        case 0b10001: set_register_value(reg1, get_register_value(reg1) - get_register_value(reg2)); break;
        case 0b10010: set_register_value(reg1, get_register_value(reg2)); break;

        // Logical & Immediate Arithmetic
        case 0b10011: set_register_value(reg1, get_register_value(reg1) + value); break;
        case 0b10100: set_register_value(reg1, get_register_value(reg1) - value); break;
        case 0b10101: {
            int result = get_register_value(reg1) - value;
            flags.ZF = (result == 0);
            flags.SF = (result < 0);
            break;
        }
        case 0b10110: set_register_value(reg1, ~get_register_value(reg1)); break;

        // Comparison & Jumps
        case 0b10111: {
            int result = get_register_value(reg1) - get_register_value(reg2);
            flags.ZF = (result == 0);
            flags.SF = (result < 0);
            break;
        }
        case 0b11000: return addr;
        case 0b11001: if (flags.ZF) return addr; break;
        case 0b11010: if (!flags.ZF) return addr; break;
        case 0b11011: if (!flags.ZF && !flags.SF) return addr; break;
        case 0b11100: if (flags.SF) return addr; break;
        case 0b11101: if (!flags.SF) return addr; break;
        case 0b11110: if (flags.ZF || flags.SF) return addr; break;

        // Stack & Functions
        case 0b01011: registers.ESP--; write_memory(registers.ESP, get_register_value(reg1)); break;
        case 0b01100: set_register_value(reg1, read_memory(registers.ESP)); registers.ESP++; break;
        case 0b01101: registers.ESP--; write_memory(registers.ESP, pc + 1); return addr;
        case 0b01110: {
            int ret_addr = read_memory(registers.ESP);
            registers.ESP++;
            return ret_addr;
        }

        default:
            fprintf(stderr, "[Runtime Error] Unknown opcode 0x%X at PC %d.\n", opcode, pc);
            return -1; // Halt on unknown instruction.
    }

    return pc + 1; // Advance to the next instruction.
}


// --- Utility Functions ---
int get_register_value(int reg_code) {
    switch (reg_code) {
        case 0: return registers.EAX;
        case 1: return registers.EBX;
        case 2: return registers.ECX;
        case 3: return registers.EDX;
        case 4: return registers.ESI;
        case 5: return registers.EDI;
        case 6: return registers.EBP;
        case 7: return registers.ESP;
        default: return 0;
    }
}

void set_register_value(int reg_code, int value) {
    switch (reg_code) {
        case 0: registers.EAX = value; break;
        case 1: registers.EBX = value; break;
        case 2: registers.ECX = value; break;
        case 3: registers.EDX = value; break;
        case 4: registers.ESI = value; break;
        case 5: registers.EDI = value; break;
        case 6: registers.EBP = value; break;
        case 7: registers.ESP = value; break;
    }
}

void write_memory(int address, int data) {
    if (address >= 0 && address < MEMORY_SIZE) {
        memory[address] = data;
    } else {
        fprintf(stderr, "[Memory Error] Attempted to write to invalid memory address %d.\n", address);
    }
}

int read_memory(int address) {
    if (address >= 0 && address < MEMORY_SIZE) return memory[address];
    fprintf(stderr, "[Memory Error] Attempted to read invalid memory address %d.\n", address);
    return 0;
}

void dump_contents() {
    printf("\n--- CPU State Dump ---\n");
    printf("Registers: EAX=%-5d EBX=%-5d ECX=%-5d EDX=%-5d\n", registers.EAX, registers.EBX, registers.ECX, registers.EDX);
    printf("           ESI=%-5d EDI=%-5d EBP=%-5d ESP=%-5d\n", registers.ESI, registers.EDI, registers.EBP, registers.ESP);
    printf("Flags:     ZF=%d SF=%d\n", flags.ZF, flags.SF);
    printf("Memory Contents (%d words):\n", MEMORY_SIZE);
    for (int i = 0; i < MEMORY_SIZE; ++i) {
        if (i % 8 == 0) printf("  [%02d]:", i);
        printf(" %5d", memory[i]);
        if ((i + 1) % 8 == 0 || i == MEMORY_SIZE - 1) printf("\n");
    }
    printf("----------------------\n");
}
