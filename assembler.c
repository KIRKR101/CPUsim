#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

// --- Configuration Constants ---
#define PROGRAM_SIZE 256        // Maximum number of instructions in a program.
#define MAX_LINE_LENGTH 100     // Maximum characters per line of assembly code.
#define MAX_FILENAME_LENGTH 256 // Maximum length for file paths.
#define MAX_LABELS 64           // Maximum number of labels in a program.
#define MAX_LABEL_LENGTH 32     // Maximum character length of a label.

// --- Core Data Structures ---

// Represents a single entry in the symbol table, mapping a label to an address.
typedef struct {
    char name[MAX_LABEL_LENGTH];
    int address;
} Label;

// --- Global State Variables ---

char  program_memory[PROGRAM_SIZE][MAX_LINE_LENGTH]; // Buffer for the program's assembly instructions.
Label symbol_table[MAX_LABELS];                      // Symbol table for labels and their addresses.
int   program_line_count = 0;                        // The number of lines in the loaded program.
int   label_count = 0;                               // The number of labels in the symbol table.

// --- Function Prototypes ---
int  load_program(const char* filename);
void build_symbol_table();
int  get_address_for_label(const char* name);
int  assemble(uint16_t* machine_code);
uint16_t encode_instruction(const char* line, int pc);
int  write_binary_file(const char* filename, const uint16_t* machine_code, int instruction_count);
int  get_register_code(const char* reg_name);


int main(int argc, char* argv[]) {
    char source_filename[MAX_FILENAME_LENGTH];
    char binary_filename[MAX_FILENAME_LENGTH];
    uint16_t machine_code[PROGRAM_SIZE] = { 0 };
    int instruction_count = 0;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source file> <output file>\n", argv[0]);
        return 1;
    }

    strncpy(source_filename, argv[1], sizeof(source_filename) - 1);
    source_filename[sizeof(source_filename) - 1] = '\0';

    strncpy(binary_filename, argv[2], sizeof(binary_filename) - 1);
    binary_filename[sizeof(binary_filename) - 1] = '\0';

    // Load the program from the source file.
    printf("\n[Pass 1] Loading source file '%s'...\n", source_filename);
    if (load_program(source_filename) < 0) {
        fprintf(stderr, "[Fatal Error] Program loading failed. Exiting.\n");
        return 1;
    }

    // Build the symbol table in the first pass.
    printf("[Pass 1] Building symbol table for labels...\n");
    build_symbol_table();
    printf("[Pass 1] Found %d labels.\n", label_count);

    // Assemble the program into machine code in the second pass.
    printf("[Pass 2] Assembling into machine code...\n");
    instruction_count = assemble(machine_code);
    if (instruction_count < 0) {
        fprintf(stderr, "[Fatal Error] Assembly failed. Please check source file for errors.\n");
        return 1;
    }
    printf("[Pass 2] Assembly successful. %d instructions generated.\n", instruction_count);

    // Write the machine code to the binary file.
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
        line_buffer[strcspn(line_buffer, "\r\n")] = 0; // Remove newline.

        // Remove comments, which start with a semicolon.
        char* comment_start = strchr(line_buffer, ';');
        if (comment_start != NULL) {
            *comment_start = '\0';
        }

        // Trim leading whitespace.
        char* trimmed_line = line_buffer;
        while (isspace((unsigned char)*trimmed_line)) {
            trimmed_line++;
        }

        // Skip empty lines.
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
    int instruction_address = 0;
    for (int i = 0; i < program_line_count; i++) {
        char line_copy[MAX_LINE_LENGTH];
        strcpy(line_copy, program_memory[i]);

        char* colon = strchr(line_copy, ':');
        if (colon != NULL) {
            *colon = '\0'; // Terminate the string at the colon to isolate the label name.

            // Add the label to the symbol table.
            if (label_count < MAX_LABELS) {
                strncpy(symbol_table[label_count].name, line_copy, MAX_LABEL_LENGTH - 1);
                symbol_table[label_count].name[MAX_LABEL_LENGTH - 1] = '\0'; // Ensure null-termination.
                symbol_table[label_count].address = instruction_address;
                label_count++;
            } else {
                fprintf(stderr, "[Warning] Maximum number of labels (%d) reached. Ignoring '%s'.\n", MAX_LABELS, line_copy);
            }

            // Remove the label from the instruction for the second pass.
            char* instruction_start = colon + 1;
            while (isspace((unsigned char)*instruction_start)) {
                instruction_start++;
            }
            strcpy(program_memory[i], instruction_start);
        }

        // If the line is not empty after stripping a label, it's an instruction.
        if (program_memory[i][0] != '\0') {
            instruction_address++;
        }
    }
}

int get_address_for_label(const char* name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return symbol_table[i].address;
        }
    }
    return -1; // Label not found.
}

int assemble(uint16_t* machine_code) {
    int instruction_count = 0;
    for (int i = 0; i < program_line_count; i++) {
        // Skip empty lines (e.g., lines that only contained a label).
        if (program_memory[i][0] == '\0') {
            continue;
        }

        // Use 0xFFFF as a sentinel for an encoding error.
        uint16_t instruction = encode_instruction(program_memory[i], i);
        if (instruction == 0xFFFF) {
            return -1; // Halt assembly on error.
        }
        machine_code[instruction_count] = instruction;
        // Print the generated machine code for each instruction.
        printf("  L%03d: %-25s -> 0x%04X\n", instruction_count, program_memory[i], instruction);
        instruction_count++;
    }
    return instruction_count;
}

int get_register_code(const char* reg_name) {
    if (reg_name == NULL) return -1;
    char upper_name[MAX_LABEL_LENGTH];
    strncpy(upper_name, reg_name, MAX_LABEL_LENGTH - 1);
    upper_name[MAX_LABEL_LENGTH - 1] = '\0';
    for (char* p = upper_name; *p; ++p) *p = toupper(*p);

    if (strcmp(upper_name, "EAX") == 0) return 0;
    if (strcmp(upper_name, "EBX") == 0) return 1;
    if (strcmp(upper_name, "ECX") == 0) return 2;
    if (strcmp(upper_name, "EDX") == 0) return 3;
    if (strcmp(upper_name, "ESI") == 0) return 4;
    if (strcmp(upper_name, "EDI") == 0) return 5;
    if (strcmp(upper_name, "EBP") == 0) return 6;
    if (strcmp(upper_name, "ESP") == 0) return 7;
    return -1; // Invalid register.
}
uint16_t encode_instruction(const char* line, int pc) {
    char line_copy[MAX_LINE_LENGTH];
    strncpy(line_copy, line, MAX_LINE_LENGTH - 1);
    line_copy[MAX_LINE_LENGTH - 1] = '\0';
    char* parts[4] = { NULL };
    int part_count = 0;
    char* token = strtok(line_copy, " \t,");
    while (token != NULL && part_count < 4) {
        parts[part_count++] = token;
        token = strtok(NULL, " \t,");
    }

    if (part_count == 0) return 0;

    char* opcode_str = parts[0];
    for (char* p = opcode_str; *p; ++p) *p = toupper(*p);

    uint16_t instruction = 0, opcode = 0;
    int reg1_code = 0, reg2_code = 0, value = 0;

    // 0-operand instructions.
    if (strcmp(opcode_str, "HLT") == 0) { opcode = 0b00000; instruction = opcode << 11; }
    else if (strcmp(opcode_str, "RET") == 0) { opcode = 0b01110; instruction = opcode << 11; }

    // 1-operand instructions (register).
    else if (strcmp(opcode_str, "INP") == 0 || strcmp(opcode_str, "OUT") == 0 ||
             strcmp(opcode_str, "INC") == 0 || strcmp(opcode_str, "DEC") == 0 ||
             strcmp(opcode_str, "NOT") == 0 || strcmp(opcode_str, "PUSH") == 0 ||
             strcmp(opcode_str, "POP") == 0) {
        if (part_count != 2) { fprintf(stderr, "[Error L%d] %s requires 1 register operand.\n", pc, opcode_str); return 0xFFFF; }
        if (strcmp(opcode_str, "INP") == 0) opcode = 0b00100;
        if (strcmp(opcode_str, "OUT") == 0) opcode = 0b00101;
        if (strcmp(opcode_str, "INC") == 0) opcode = 0b01001;
        if (strcmp(opcode_str, "DEC") == 0) opcode = 0b01010;
        if (strcmp(opcode_str, "PUSH") == 0) opcode = 0b01011;
        if (strcmp(opcode_str, "POP") == 0) opcode = 0b01100;
        if (strcmp(opcode_str, "NOT") == 0) opcode = 0b10110;
        reg1_code = get_register_code(parts[1]);
        if (reg1_code == -1) { fprintf(stderr, "[Error L%d] Invalid register '%s'.\n", pc, parts[1]); return 0xFFFF; }
        instruction = (opcode << 11) | (reg1_code << 8);
    }

    // 1-operand instructions (address/label).
    else if (strcmp(opcode_str, "JMP") == 0 || strcmp(opcode_str, "JE") == 0 || strcmp(opcode_str, "JZ") == 0 ||
             strcmp(opcode_str, "JNE") == 0 || strcmp(opcode_str, "JNZ") == 0 || strcmp(opcode_str, "JG") == 0 ||
             strcmp(opcode_str, "JNLE") == 0 || strcmp(opcode_str, "JL") == 0 || strcmp(opcode_str, "JNGE") == 0 ||
             strcmp(opcode_str, "JGE") == 0 || strcmp(opcode_str, "JNL") == 0 || strcmp(opcode_str, "JLE") == 0 ||
             strcmp(opcode_str, "JNG") == 0 || strcmp(opcode_str, "CALL") == 0) {
        if (part_count != 2) { fprintf(stderr, "[Error L%d] %s requires 1 address/label operand.\n", pc, opcode_str); return 0xFFFF; }
        if (strcmp(opcode_str, "JMP") == 0) opcode = 0b11000;
        if (strcmp(opcode_str, "JE") == 0 || strcmp(opcode_str, "JZ") == 0) opcode = 0b11001;
        if (strcmp(opcode_str, "JNE") == 0 || strcmp(opcode_str, "JNZ") == 0) opcode = 0b11010;
        if (strcmp(opcode_str, "JG") == 0 || strcmp(opcode_str, "JNLE") == 0) opcode = 0b11011;
        if (strcmp(opcode_str, "JL") == 0 || strcmp(opcode_str, "JNGE") == 0) opcode = 0b11100;
        if (strcmp(opcode_str, "JGE") == 0 || strcmp(opcode_str, "JNL") == 0) opcode = 0b11101;
        if (strcmp(opcode_str, "JLE") == 0 || strcmp(opcode_str, "JNG") == 0) opcode = 0b11110;
        if (strcmp(opcode_str, "CALL") == 0) opcode = 0b01101;
        value = isalpha((unsigned char)parts[1][0]) ? get_address_for_label(parts[1]) : atoi(parts[1]);
        if (value == -1) { fprintf(stderr, "[Error L%d] Undefined label '%s'.\n", pc, parts[1]); return 0xFFFF; }
        if (value < 0 || value > 0xFF) { fprintf(stderr, "[Error L%d] Address %d out of range (0-255).\n", pc, value); return 0xFFFF; }
        instruction = (opcode << 11) | value;
    }

    // 2-operand instructions.
    else if (strcmp(opcode_str, "ADD") == 0 || strcmp(opcode_str, "SUB") == 0 || strcmp(opcode_str, "CMP") == 0 ||
             strcmp(opcode_str, "MUL") == 0 || strcmp(opcode_str, "DIV") == 0 || strcmp(opcode_str, "XOR") == 0) {
        if (part_count != 3) { fprintf(stderr, "[Error L%d] %s requires 2 operands.\n", pc, opcode_str); return 0xFFFF; }
        char* operand2 = parts[2];

        // Check if the second operand is an immediate value.
        if (operand2[0] == '#') {
            if (strcmp(opcode_str, "ADD") == 0) opcode = 0b10011;
            else if (strcmp(opcode_str, "SUB") == 0) opcode = 0b10100;
            else if (strcmp(opcode_str, "CMP") == 0) opcode = 0b10101;
            else { fprintf(stderr, "[Error L%d] Immediate value not supported for %s.\n", pc, opcode_str); return 0xFFFF; }

            reg1_code = get_register_code(parts[1]);
            value = atoi(operand2 + 1);
            if (reg1_code == -1) { fprintf(stderr, "[Error L%d] Invalid register '%s'.\n", pc, parts[1]); return 0xFFFF; }
            if (value < 0 || value > 0xFF) { fprintf(stderr, "[Error L%d] Immediate value %d out of range (0-255).\n", pc, value); return 0xFFFF; }
            instruction = (opcode << 11) | (reg1_code << 8) | value;
        }
        // Otherwise, it's a register-register operation.
        else {
            if (strcmp(opcode_str, "ADD") == 0) opcode = 0b10000;
            if (strcmp(opcode_str, "SUB") == 0) opcode = 0b10001;
            if (strcmp(opcode_str, "CMP") == 0) opcode = 0b10111;
            if (strcmp(opcode_str, "MUL") == 0) opcode = 0b00001;
            if (strcmp(opcode_str, "DIV") == 0) opcode = 0b00010;
            if (strcmp(opcode_str, "XOR") == 0) opcode = 0b00011;
            reg1_code = get_register_code(parts[1]);
            reg2_code = get_register_code(parts[2]);
            if (reg1_code == -1 || reg2_code == -1) { fprintf(stderr, "[Error L%d] Invalid register in %s instruction.\n", pc, opcode_str); return 0xFFFF; }
            instruction = (opcode << 11) | (reg1_code << 8) | (reg2_code << 5);
        }
    }

    // The MOV instruction has special handling due to its multiple forms.
    else if (strcmp(opcode_str, "MOV") == 0) {
        if (part_count != 3) { fprintf(stderr, "[Error L%d] MOV requires 2 operands.\n", pc); return 0xFFFF; }
        char* dest = parts[1]; char* src = parts[2];
        int dest_is_mem = (dest[0] == '['); int src_is_mem = (src[0] == '[');

        if (dest_is_mem && src_is_mem) { fprintf(stderr, "[Error L%d] Memory-to-memory MOV is not supported.\n", pc); return 0xFFFF; }

        // Check for base+offset addressing, e.g., [EBP+1].
        char* plus = strchr(dest_is_mem ? dest : src, '+');
        if ((dest_is_mem || src_is_mem) && plus != NULL) {
            char base_reg_str[MAX_LABEL_LENGTH];
            int offset = 0;
            int base_reg_code = -1;
            int reg_code = -1;

            char* mem_operand = dest_is_mem ? dest : src;
            // Extract the base register and offset.
            strncpy(base_reg_str, mem_operand + 1, plus - (mem_operand + 1));
            base_reg_str[plus - (mem_operand + 1)] = '\0';
            offset = atoi(plus + 1);
            base_reg_code = get_register_code(base_reg_str);

            if (base_reg_code == -1) { fprintf(stderr, "[Error L%d] Invalid base register '%s' in memory operand.\n", pc, base_reg_str); return 0xFFFF; }
            if (offset < 0 || offset > 0x1F) { fprintf(stderr, "[Error L%d] Offset %d out of range (0-31).\n", pc, offset); return 0xFFFF; }

            if (dest_is_mem) { // MOV [base+off], reg.
                opcode = 0b11111;
                reg_code = get_register_code(src);
                if (reg_code == -1) { fprintf(stderr, "[Error L%d] Invalid source register '%s'.\n", pc, src); return 0xFFFF; }
                instruction = (opcode << 11) | (reg_code << 8) | (base_reg_code << 5) | offset;
            } else { // MOV reg, [base+off].
                opcode = 0b01111;
                reg_code = get_register_code(dest);
                if (reg_code == -1) { fprintf(stderr, "[Error L%d] Invalid destination register '%s'.\n", pc, dest); return 0xFFFF; }
                instruction = (opcode << 11) | (reg_code << 8) | (base_reg_code << 5) | offset;
            }
        }
        else if (!dest_is_mem && src[0] == '#') { // MOV reg, imm.
            opcode = 0b00110;
            reg1_code = get_register_code(dest);
            value = atoi(src + 1);
            if (reg1_code == -1) { fprintf(stderr, "[Error L%d] Invalid register '%s'.\n", pc, dest); return 0xFFFF; }
            if (value < 0 || value > 0xFF) { fprintf(stderr, "[Error L%d] Immediate value %d out of range (0-255).\n", pc, value); return 0xFFFF; }
            instruction = (opcode << 11) | (reg1_code << 8) | value;
        }
        else if (!dest_is_mem && src_is_mem) { // MOV reg, [addr].
            opcode = 0b00111;
            reg1_code = get_register_code(dest);
            char addr_str[MAX_LABEL_LENGTH]; strncpy(addr_str, src + 1, strlen(src) - 2); addr_str[strlen(src) - 2] = '\0';
            value = isalpha((unsigned char)addr_str[0]) ? get_address_for_label(addr_str) : atoi(addr_str);
            if (reg1_code == -1) { fprintf(stderr, "[Error L%d] Invalid register '%s'.\n", pc, dest); return 0xFFFF; }
            if (value == -1) { fprintf(stderr, "[Error L%d] Undefined label '%s'.\n", pc, addr_str); return 0xFFFF; }
            if (value < 0 || value > 0xFF) { fprintf(stderr, "[Error L%d] Address %d out of range (0-255).\n", pc, value); return 0xFFFF; }
            instruction = (opcode << 11) | (reg1_code << 8) | value;
        }
        else if (dest_is_mem && !src_is_mem) { // MOV [addr], reg.
            opcode = 0b01000;
            reg1_code = get_register_code(src);
            char addr_str[MAX_LABEL_LENGTH]; strncpy(addr_str, dest + 1, strlen(dest) - 2); addr_str[strlen(dest) - 2] = '\0';
            value = isalpha((unsigned char)addr_str[0]) ? get_address_for_label(addr_str) : atoi(addr_str);
            if (reg1_code == -1) { fprintf(stderr, "[Error L%d] Invalid register '%s'.\n", pc, src); return 0xFFFF; }
            if (value == -1) { fprintf(stderr, "[Error L%d] Undefined label '%s'.\n", pc, addr_str); return 0xFFFF; }
            if (value < 0 || value > 0xFF) { fprintf(stderr, "[Error L%d] Address %d out of range (0-255).\n", pc, value); return 0xFFFF; }
            instruction = (opcode << 11) | (reg1_code << 8) | value;
        }
        else if (!dest_is_mem && !src_is_mem) { // MOV reg, reg.
            opcode = 0b10010;
            reg1_code = get_register_code(dest);
            reg2_code = get_register_code(src);
            if (reg1_code == -1 || reg2_code == -1) { fprintf(stderr, "[Error L%d] Invalid register in MOV instruction.\n", pc); return 0xFFFF; }
            instruction = (opcode << 11) | (reg1_code << 8) | (reg2_code << 5);
        }
        else { fprintf(stderr, "[Error L%d] Invalid operands for MOV: %s, %s\n", pc, dest, src); return 0xFFFF; }
    }
    else {
        fprintf(stderr, "[Error L%d] Unknown mnemonic '%s'.\n", pc, opcode_str);
        return 0xFFFF;
    }
    return instruction;
}


int write_binary_file(const char* filename, const uint16_t* machine_code, int instruction_count) {
    FILE* f = fopen(filename, "wb");
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
