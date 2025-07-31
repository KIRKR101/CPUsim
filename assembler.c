#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

// --- Configuration Constants ---
#define PROGRAM_SIZE 256        // Maximum number of instructions the program can hold.
#define MAX_LINE_LENGTH 100     // Maximum characters allowed per instruction line.
#define MAX_FILENAME_LENGTH 256 // Maximum length for input/output filenames.
#define MAX_LABELS 64           // Maximum number of labels that can be defined.
#define MAX_LABEL_LENGTH 32     // Maximum character length of a label.

// --- Core Data Structures ---

/**
 * @brief Represents a single entry in the symbol table (a label and its address).
 */
typedef struct {
    char name[MAX_LABEL_LENGTH];
    int address;
} Label;

// --- Global State Variables ---

char  program_memory[PROGRAM_SIZE][MAX_LINE_LENGTH]; // Stores the loaded program instructions as strings.
Label symbol_table[MAX_LABELS];                      // Stores labels and their addresses.
int   program_line_count = 0;                        // Actual number of instructions loaded.
int   label_count = 0;                               // Number of labels found.

// --- Function Prototypes ---
int  load_program(const char* filename);
void build_symbol_table();
int  get_address_for_label(const char* name);
int  assemble(uint16_t* machine_code);
uint16_t encode_instruction(const char* line, int pc);
int  write_binary_file(const char* filename, const uint16_t* machine_code, int instruction_count);
int  get_register_code(const char* reg_name);


int main() {
    char source_filename[MAX_FILENAME_LENGTH];
    char binary_filename[MAX_FILENAME_LENGTH];
    uint16_t machine_code[PROGRAM_SIZE] = { 0 };
    int instruction_count = 0;

    // --- Get Filenames ---
    printf("--- Simple 16-bit Assembler ---\n");
    printf("Enter the source assembly filename (e.g., program.txt): ");
    if (!fgets(source_filename, sizeof(source_filename), stdin)) {
        fprintf(stderr, "[Fatal Error] Failed to read source filename.\n");
        return 1;
    }
    source_filename[strcspn(source_filename, "\r\n")] = 0;

    printf("Enter the output binary filename (e.g., program.bin): ");
    if (!fgets(binary_filename, sizeof(binary_filename), stdin)) {
        fprintf(stderr, "[Fatal Error] Failed to read output filename.\n");
        return 1;
    }
    binary_filename[strcspn(binary_filename, "\r\n")] = 0;

    // --- Step 1: Load Program from Source File ---
    printf("\n[Pass 1] Loading source file '%s'...\n", source_filename);
    if (load_program(source_filename) < 0) {
        fprintf(stderr, "[Fatal Error] Program loading failed. Exiting.\n");
        return 1;
    }

    // --- Step 2: Build Symbol Table (First Pass) ---
    printf("[Pass 1] Building symbol table for labels...\n");
    build_symbol_table();
    printf("[Pass 1] Found %d labels.\n", label_count);

    // --- Step 3: Assemble into Machine Code (Second Pass) ---
    printf("[Pass 2] Assembling into machine code...\n");
    instruction_count = assemble(machine_code);
    if (instruction_count < 0) {
        fprintf(stderr, "[Fatal Error] Assembly failed. Please check source file for errors.\n");
        return 1;
    }
    printf("[Pass 2] Assembly successful. %d instructions generated.\n", instruction_count);

    // --- Step 4: Write Machine Code to Binary File ---
    printf("\nWriting %d words to binary file '%s'...\n", instruction_count, binary_filename);
    if (write_binary_file(binary_filename, machine_code, instruction_count) != 0) {
        fprintf(stderr, "[Fatal Error] Could not write to binary file.\n");
        return 1;
    }

    printf("\nAssembly complete. Binary file '%s' created successfully.\n", binary_filename);
    return 0;
}

int load_program(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        perror("[Loader Error] Failed to open program file");
        return -1;
    }

    int i = 0;
    char line_buffer[MAX_LINE_LENGTH];

    while (i < PROGRAM_SIZE && fgets(line_buffer, MAX_LINE_LENGTH, f) != NULL) {
        line_buffer[strcspn(line_buffer, "\r\n")] = 0; // Remove newline

        // Remove comments (anything after ';')
        char* comment_start = strchr(line_buffer, ';');
        if (comment_start != NULL) {
            *comment_start = '\0';
        }

        // Trim leading whitespace
        char* trimmed_line = line_buffer;
        while (isspace((unsigned char)*trimmed_line)) {
            trimmed_line++;
        }

        // Skip lines that are now empty
        if (*trimmed_line == '\0') {
            continue;
        }

        strncpy(program_memory[i], trimmed_line, MAX_LINE_LENGTH - 1);
        i++;
    }

    fclose(f);
    program_line_count = i;
    return program_line_count;
}

void build_symbol_table() {
    label_count = 0;
    for (int i = 0; i < program_line_count; i++) {
        char line_copy[MAX_LINE_LENGTH];
        strcpy(line_copy, program_memory[i]);

        char* colon = strchr(line_copy, ':');
        if (colon != NULL) {
            *colon = '\0'; // Terminate the string at the colon to isolate the label name

            // Add to symbol table
            if (label_count < MAX_LABELS) {
                strncpy(symbol_table[label_count].name, line_copy, MAX_LABEL_LENGTH - 1);
                strncpy(symbol_table[label_count].name, line_copy, MAX_LABEL_LENGTH - 1);
                symbol_table[label_count].name[MAX_LABEL_LENGTH - 1] = '\0'; // Ensure null-termination
                symbol_table[label_count].address = i; // The address is the current line number
                label_count++;
                fprintf(stderr, "[Warning] Maximum number of labels (%d) reached. Ignoring '%s'.\n", MAX_LABELS, line_copy);
            }

            // Remove the label from the in-memory instruction for the second pass
            char* instruction_start = colon + 1;
            while (isspace((unsigned char)*instruction_start)) {
                instruction_start++;
            }
            strcpy(program_memory[i], instruction_start);
        }
    }
}

int get_address_for_label(const char* name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return symbol_table[i].address;
        }
    }
    return -1; // Not found
}

int assemble(uint16_t* machine_code) {
    for (int i = 0; i < program_line_count; i++) {
        // Use 0xFFFF as a sentinel for an error during encoding
        uint16_t instruction = encode_instruction(program_memory[i], i);
        if (instruction == 0xFFFF) {
            return -1; // Halt assembly on error
        }
        machine_code[i] = instruction;
        // Debug print to show the generated code for each line
        printf("  L%03d: %-25s -> 0x%04X\n", i, program_memory[i], instruction);
    }
    return program_line_count;
}

int get_register_code(const char* reg_name) {
    if (reg_name == NULL) return -1;
    if (strcmp(reg_name, "A") == 0) return 0; // 0b00
    if (strcmp(reg_name, "B") == 0) return 1; // 0b01
    if (strcmp(reg_name, "C") == 0) return 2; // 0b10
    return -1; // Invalid register
}
uint16_t encode_instruction(const char* line, int pc) {
    char line_copy[MAX_LINE_LENGTH];
    strncpy(line_copy, line, MAX_LINE_LENGTH - 1);
    line_copy[MAX_LINE_LENGTH - 1] = '\0';
    strncpy(line_copy, line, MAX_LINE_LENGTH);

    char* parts[4] = { NULL };
    int part_count = 0;
    char* token = strtok(line_copy, " \t,");
    while (token != NULL && part_count < 4) {
        parts[part_count++] = token;
        token = strtok(NULL, " \t,");
    }

    if (part_count == 0) return 0; // Empty line, encode as NOP (or 0)

    char* opcode_str = parts[0];
    for (char* p = opcode_str; *p; ++p) *p = toupper(*p); // Uppercase for case-insensitivity

    uint16_t instruction = 0;
    uint16_t opcode = 0;
    int reg1_code = 0, reg2_code = 0;
    int value = 0;

    // --- Z-TYPE INSTRUCTIONS (0 OPERANDS) ---
    if (strcmp(opcode_str, "HLT") == 0) { opcode = 0b00000; instruction = opcode << 11; }
    else if (strcmp(opcode_str, "DMP") == 0) { opcode = 0b00001; instruction = opcode << 11; }
    else if (strcmp(opcode_str, "CLRM") == 0){ opcode = 0b00010; instruction = opcode << 11; } // Clear all memory
    else if (strcmp(opcode_str, "CLRR") == 0){ opcode = 0b00011; instruction = opcode << 11; } // Clear all registers

    // --- R-TYPE INSTRUCTIONS (2 REGISTER OPERANDS) ---
    else if (strcmp(opcode_str, "ADD") == 0 || strcmp(opcode_str, "SUB") == 0 || strcmp(opcode_str, "MOV") == 0) {
        if (part_count != 3) { fprintf(stderr, "[Error L%d] Instruction %s requires 2 register operands.\n", pc, opcode_str); return 0xFFFF; }
        if (strcmp(opcode_str, "ADD") == 0) opcode = 0b10000;
        if (strcmp(opcode_str, "SUB") == 0) opcode = 0b10001;
        if (strcmp(opcode_str, "MOV") == 0) opcode = 0b10010;
        reg1_code = get_register_code(parts[1]); // Destination
        reg2_code = get_register_code(parts[2]); // Source
        if (reg1_code == -1 || reg2_code == -1) { fprintf(stderr, "[Error L%d] Invalid register in %s instruction.\n", pc, opcode_str); return 0xFFFF; }
        instruction = (opcode << 11) | (reg1_code << 9) | (reg2_code << 7);
    }

    // --- I-TYPE INSTRUCTIONS (REGISTER AND/OR VALUE/ADDRESS) ---
    else {
        if (strcmp(opcode_str, "INP") == 0 || strcmp(opcode_str, "OUT") == 0 || strcmp(opcode_str, "INC") == 0 || strcmp(opcode_str, "DEC") == 0) {
            if (part_count != 2) { fprintf(stderr, "[Error L%d] Instruction %s requires 1 register operand.\n", pc, opcode_str); return 0xFFFF; }
            if (strcmp(opcode_str, "INP") == 0) opcode = 0b00100;
            if (strcmp(opcode_str, "OUT") == 0) opcode = 0b00101;
            if (strcmp(opcode_str, "INC") == 0) opcode = 0b01001;
            if (strcmp(opcode_str, "DEC") == 0) opcode = 0b01010;
            reg1_code = get_register_code(parts[1]);
            if (reg1_code == -1) { fprintf(stderr, "[Error L%d] Invalid register '%s'.\n", pc, parts[1]); return 0xFFFF; }
            instruction = (opcode << 11) | (reg1_code << 9);
        }
        else if (strcmp(opcode_str, "SET") == 0) {
            if (part_count != 3) { fprintf(stderr, "[Error L%d] SET requires a register and an immediate value (e.g., SET A, #123).\n", pc); return 0xFFFF; }
            opcode = 0b00110;
            reg1_code = get_register_code(parts[1]);
            if (parts[2][0] != '#') { fprintf(stderr, "[Error L%d] SET requires an immediate value starting with #.\n", pc); return 0xFFFF; }
            value = atoi(parts[2] + 1);
            if (reg1_code == -1) { fprintf(stderr, "[Error L%d] Invalid register '%s'.\n", pc, parts[1]); return 0xFFFF; }
            if (value < 0 || value > 0x1FF) { fprintf(stderr, "[Error L%d] Immediate value %d out of range (0-511).\n", pc, value); return 0xFFFF; }
            instruction = (opcode << 11) | (reg1_code << 9) | value;
        }
        else if (strcmp(opcode_str, "LDA") == 0 || strcmp(opcode_str, "STA") == 0) {
            if (part_count != 3) { fprintf(stderr, "[Error L%d] %s requires a register and an address/label.\n", pc, opcode_str); return 0xFFFF; }
            if (strcmp(opcode_str, "LDA") == 0) opcode = 0b00111;
            else opcode = 0b01000;
            reg1_code = get_register_code(parts[1]);
            value = isalpha((unsigned char)parts[2][0]) ? get_address_for_label(parts[2]) : atoi(parts[2]);
            if (reg1_code == -1) { fprintf(stderr, "[Error L%d] Invalid register '%s'.\n", pc, parts[1]); return 0xFFFF; }
            if (value == -1) { fprintf(stderr, "[Error L%d] Undefined label '%s'.\n", pc, parts[2]); return 0xFFFF; }
            if (value < 0 || value > 0x1FF) { fprintf(stderr, "[Error L%d] Address %d out of range (0-511).\n", pc, value); return 0xFFFF; }
            instruction = (opcode << 11) | (reg1_code << 9) | value;
        }
        else if (strcmp(opcode_str, "JMP") == 0) {
            if (part_count != 2) { fprintf(stderr, "[Error L%d] JMP requires 1 address/label operand.\n", pc); return 0xFFFF; }
            opcode = 0b11000;
            value = isalpha((unsigned char)parts[1][0]) ? get_address_for_label(parts[1]) : atoi(parts[1]);
            if (value == -1) { fprintf(stderr, "[Error L%d] Undefined label '%s'.\n", pc, parts[1]); return 0xFFFF; }
            if (value < 0 || value > 0x1FF) { fprintf(stderr, "[Error L%d] Jump address %d out of range (0-511).\n", pc, value); return 0xFFFF; }
            instruction = (opcode << 11) | value;
        }
        else if (strcmp(opcode_str, "JZ") == 0 || strcmp(opcode_str, "JNZ") == 0 || strcmp(opcode_str, "JP") == 0 || strcmp(opcode_str, "JN") == 0) {
            if (part_count != 3) { fprintf(stderr, "[Error L%d] %s requires a register and an address/label.\n", pc, opcode_str); return 0xFFFF; }
            if (strcmp(opcode_str, "JZ") == 0) opcode = 0b11001;
            if (strcmp(opcode_str, "JNZ") == 0) opcode = 0b11010;
            if (strcmp(opcode_str, "JP") == 0) opcode = 0b11011;
            if (strcmp(opcode_str, "JN") == 0) opcode = 0b11100;
            reg1_code = get_register_code(parts[1]);
            value = isalpha((unsigned char)parts[2][0]) ? get_address_for_label(parts[2]) : atoi(parts[2]);
            if (reg1_code == -1) { fprintf(stderr, "[Error L%d] Invalid register '%s'.\n", pc, parts[1]); return 0xFFFF; }
            if (value == -1) { fprintf(stderr, "[Error L%d] Undefined label '%s'.\n", pc, parts[2]); return 0xFFFF; }
            if (value < 0 || value > 0x1FF) { fprintf(stderr, "[Error L%d] Jump address %d out of range (0-511).\n", pc, value); return 0xFFFF; }
            instruction = (opcode << 11) | (reg1_code << 9) | value;
        }
        else {
            fprintf(stderr, "[Error L%d] Unknown instruction mnemonic '%s'.\n", pc, opcode_str);
            return 0xFFFF;
        }
    }
    return instruction;
}


int write_binary_file(const char* filename, const uint16_t* machine_code, int instruction_count) {
    FILE* f = fopen(filename, "wb"); // write binary
    if (f == NULL) {
        perror("[File Error] Failed to open binary file for writing");
        return -1;
    }

    size_t written = fwrite(machine_code, sizeof(uint16_t), instruction_count, f);
    fclose(f);

    if (written != instruction_count) {
        fprintf(stderr, "[File Error] Did not write all instructions to file.\n");
        return -1;
    }
    return 0;
}
