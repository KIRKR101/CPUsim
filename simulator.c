#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // For uint16_t

// --- Configuration Constants ---
#define MEMORY_SIZE 64          // Must match the architecture your code was assembled for
#define PROGRAM_SIZE 256        // Maximum number of instructions
#define MAX_FILENAME_LENGTH 256
#define NUM_REGISTERS 3

// --- Core Data Structures ---
typedef struct {
    int A;
    int B;
    int C;
} Registers;

// --- Global State ---
Registers registers = { 0, 0, 0 };
int memory[MEMORY_SIZE];
uint16_t machine_code[PROGRAM_SIZE] = { 0 };
int program_instruction_count = 0;

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
int main() {
    char binary_filename[MAX_FILENAME_LENGTH];

    printf("--- 16-bit CPU Simulator ---\n");
    printf("Enter the binary program filename to run (e.g., program.bin): ");
    if (!fgets(binary_filename, sizeof(binary_filename), stdin)) {
        fprintf(stderr, "[Fatal Error] Failed to read filename.\n");
        return 1;
    }
    binary_filename[strcspn(binary_filename, "\r\n")] = 0;

    if (load_binary_program(binary_filename) < 0) {
        fprintf(stderr, "[Fatal Error] Could not load binary file. Exiting.\n");
        return 1;
    }

    printf("\nStarting program execution...\n-----------------------------\n");
    run_program();

    printf("\n-----------------------------\nExecution finished.\n");
    dump_contents(); // Dump final state

    return 0;
}

int load_binary_program(const char* filename) {
    FILE* f = fopen(filename, "rb"); // Open in "read binary" mode
    if (f == NULL) {
        perror("[Loader Error] Failed to open binary file");
        return -1;
    }

    // Read the entire file into the machine_code buffer
    // fread returns the number of items read.
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
    int pc = 0; // Program Counter starts at 0
    memset(memory, 0, sizeof(memory)); // Clear main memory before run

    while (pc >= 0 && pc < program_instruction_count) {
        int next_pc = execute_instruction(machine_code[pc], pc);
        pc = next_pc;
    }

    if (pc >= program_instruction_count) {
        printf("--- Program finished: Reached end of instructions ---\n");
    }
}

int execute_instruction(uint16_t instruction, int pc) {
    // Decode the instruction using bitwise operations
    uint16_t opcode = instruction >> 11;             // Get top 5 bits
    uint16_t reg1 = (instruction >> 9) & 0x03;       // Get bits 10-9 (0b11 = 3)
    uint16_t reg2 = (instruction >> 7) & 0x03;       // Get bits 8-7
    uint16_t value = instruction & 0x1FF;            // Get bottom 9 bits for I-Type
    uint16_t addr = instruction & 0x1FF;             // Alias for clarity

    // --- INSTRUCTION DISPATCH ---
    switch (opcode) {
        case 0b00000: /* HLT */ printf("--- HLT instruction at PC %d ---\n", pc); return -1;
        case 0b00001: /* DMP */ dump_contents(); break;
        case 0b00010: /* CLRM */ memset(memory, 0, sizeof(memory)); break;
        case 0b00011: /* CLRR */ registers.A = registers.B = registers.C = 0; break;

        case 0b00100: /* INP */ {
            int input_val;
            printf("INPUT required for register %c: ", 'A' + reg1);
            if (scanf("%d", &input_val) != 1) {
                fprintf(stderr, "[Runtime Error] Invalid integer input.\n");
                while (getchar() != '\n'); // Clear buffer
                set_register_value(reg1, 0);
            } else {
                set_register_value(reg1, input_val);
                while (getchar() != '\n'); // Clear buffer
            }
            break;
        }
        case 0b00101: /* OUT */ printf("OUTPUT from register %c: %d\n", 'A' + reg1, get_register_value(reg1)); break;
        case 0b00110: /* SET */ set_register_value(reg1, value); break;
        case 0b00111: /* LDA */ set_register_value(reg1, read_memory(addr)); break;
        case 0b01000: /* STA */ write_memory(addr, get_register_value(reg1)); break;

        case 0b01001: /* INC */ set_register_value(reg1, get_register_value(reg1) + 1); break;
        case 0b01010: /* DEC */ set_register_value(reg1, get_register_value(reg1) - 1); break;

        case 0b10000: /* ADD */ set_register_value(reg1, get_register_value(reg1) + get_register_value(reg2)); break;
        case 0b10001: /* SUB */ set_register_value(reg1, get_register_value(reg1) - get_register_value(reg2)); break;
        case 0b10010: /* MOV */ set_register_value(reg1, get_register_value(reg2)); break;

        case 0b11000: /* JMP */ return addr;
        case 0b11001: /* JZ  */ if (get_register_value(reg1) == 0) return addr; break;
        case 0b11010: /* JNZ */ if (get_register_value(reg1) != 0) return addr; break;
        case 0b11011: /* JP  */ if (get_register_value(reg1) > 0)  return addr; break;
        case 0b11100: /* JN  */ if (get_register_value(reg1) < 0)  return addr; break;

        default:
            fprintf(stderr, "[Runtime Error] Unknown opcode 0x%X at PC %d.\n", opcode, pc);
            return -1; // Halt on unknown instruction
    }

    return pc + 1; // Default: advance to next instruction
}


// --- Utility Functions ---
int get_register_value(int reg_code) {
    if (reg_code == 0) return registers.A;
    if (reg_code == 1) return registers.B;
    if (reg_code == 2) return registers.C;
    return 0;
}

void set_register_value(int reg_code, int value) {
    if (reg_code == 0) registers.A = value;
    else if (reg_code == 1) registers.B = value;
    else if (reg_code == 2) registers.C = value;
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
    printf("Registers: A=%-5d B=%-5d C=%-5d\n", registers.A, registers.B, registers.C);
    printf("Memory Contents (%d words):\n", MEMORY_SIZE);
    for (int i = 0; i < MEMORY_SIZE; ++i) {
        if (i % 8 == 0) printf("  [%02d]:", i);
        printf(" %5d", memory[i]);
        if ((i + 1) % 8 == 0 || i == MEMORY_SIZE - 1) printf("\n");
    }
    printf("----------------------\n");
}
