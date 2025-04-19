#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// --- Configuration Constants ---

#define MEMORY_SIZE 64          // Total number of integer words in main memory.
#define PROGRAM_SIZE 64         // Maximum number of instructions the program memory can hold.
#define MAX_LINE_LENGTH 100     // Maximum characters allowed per instruction line in the source file.
#define MAX_FILENAME_LENGTH 256 // Maximum length for input filenames.
#define NUM_REGISTERS 3         // Number of general-purpose registers (A, B, C).

// --- Core Data Structures ---

/**
 * @brief Represents the CPU's general-purpose registers.
 *
 * Encapsulates the primary working storage accessible directly by instructions.
 */
typedef struct {
    int A; // Accumulator/General Purpose Register A
    int B; // General Purpose Register B
    int C; // General Purpose Register C
} Registers;

// --- Global State Variables ---

Registers registers = { 0, 0, 0 };              // CPU registers, initialized to zero at startup.
int memory[MEMORY_SIZE];                        // Simulated main memory array.
char program_memory[PROGRAM_SIZE][MAX_LINE_LENGTH]; // Stores the loaded program instructions as strings.
int program_line_count = 0;                     // Actual number of instructions loaded into program_memory.
int debug = 0;                                  // Flag to enable/disable verbose debugging output (0=off, 1=on).

// --- Instruction Set Reference ---

const char* instruction_set[] = {
    "SET", "STA", "LDA", "INP", "ADD", "SUB", "JMP", "JZ", "JNZ",
    "JP", "JN", "INC", "DEC", "CLRR", "CLRM", "OUT", "HLT", "DMP",
    "MOV" // Added based on observed usage/implementation
};
// Calculate the size of the instruction set array dynamically.
const int instruction_set_size = sizeof(instruction_set) / sizeof(instruction_set[0]);

// --- Function Prototypes ---
// Forward declarations for functions

int read_memory(int address);
void write_memory(int address, int data);
int load_program(const char* filename);
int run_instruction(const char* line, int pc);
void run_program();
void dump_contents();
int get_register_value(const char* reg_name);
void set_register_value(const char* reg_name, int value);
int parse_operand(const char* operand_str, int* is_immediate);


// --- Function Implementations ---

/**
 * @brief Reads a value from the specified memory address.
 * @param address The memory address to read from.
 * @return The value stored at the address, or 0 if the address is invalid.
 * @note Includes bounds checking to prevent accessing out-of-range memory.
 *       Logs an error to stderr for invalid accesses.
 */
int read_memory(int address) {
    if (address >= 0 && address < MEMORY_SIZE) {
        return memory[address];
    }
    else {
        fprintf(stderr, "[Runtime Error] Invalid memory read attempt at address: %d. Bounds are [0, %d).\n",
            address, MEMORY_SIZE);
        // Error handling choice: return 0 for reads. Could alternatively halt execution.
        return 0;
    }
}

/**
 * @brief Writes a value to the specified memory address.
 * @param address The memory address to write to.
 * @param data The integer value to store.
 * @note Includes bounds checking to prevent writing out-of-range memory.
 *       Logs an error to stderr for invalid accesses.
 */
void write_memory(int address, int data) {
    if (address >= 0 && address < MEMORY_SIZE) {
        memory[address] = data;
    }
    else {
        fprintf(stderr, "[Runtime Error] Invalid memory write attempt at address: %d. Bounds are [0, %d).\n",
            address, MEMORY_SIZE);
        // Error handling choice: Log error and continue. Could alternatively halt execution.
    }
}

/**
 * @brief Loads a program from a text file into the program memory buffer.
 * @param filename The path to the program file.
 * @return The number of instructions successfully loaded, or -1 if the file cannot be opened.
 * @note Reads the file line by line, stores instructions in `program_memory`.
 *       Removes trailing newline characters.
 *       Skips empty lines and lines starting with ';' (comments).
 *       Updates the global `program_line_count`.
 */
int load_program(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        perror("[Loader Error] Failed to open program file"); // perror provides system error message
        return -1;
    }

    int i = 0; // Index for program_memory
    char line_buffer[MAX_LINE_LENGTH]; // Temporary buffer to read lines into

    while (i < PROGRAM_SIZE && fgets(line_buffer, MAX_LINE_LENGTH, f) != NULL) {
        // Remove trailing newline or carriage return characters for cross-platform compatibility.
        line_buffer[strcspn(line_buffer, "\r\n")] = 0;

        // --- Line Preprocessing ---
        // Skip leading whitespace to find the actual start of content.
        char* trimmed_line = line_buffer;
        while (isspace((unsigned char)*trimmed_line)) {
            trimmed_line++;
        }

        // Skip lines that are empty or designated as comments (starting with ';').
        if (*trimmed_line == '\0' || *trimmed_line == ';') {
            if (debug) {
                printf("LOADER [%d]: Skipping empty/comment line: '%s'\n", i, line_buffer);
            }
            continue; // Skip this line, effectively overwriting this slot if needed later
        }

        // Line seems valid, store it in program memory.
        strncpy(program_memory[i], trimmed_line, MAX_LINE_LENGTH - 1);
        program_memory[i][MAX_LINE_LENGTH - 1] = '\0'; // Ensure null termination

        if (debug) {
            printf("LOADED [%d]: %s\n", i, program_memory[i]);
        }
        i++; // Move to the next slot in program memory
    }

    // Check if the loop terminated due to reaching PROGRAM_SIZE limit
    if (i == PROGRAM_SIZE && fgets(line_buffer, MAX_LINE_LENGTH, f) != NULL) {
        fprintf(stderr, "[Loader Warning] Maximum program size (%d lines) reached. File '%s' may be truncated.\n", PROGRAM_SIZE, filename);
    }

    fclose(f);
    program_line_count = i; // Store the actual number of loaded instructions

    if (debug) {
        printf("--- Program Loading Complete (%d instructions loaded) ---\n", program_line_count);
    }
    return program_line_count;
}

/**
 * @brief Retrieves the current value of a specified register.
 * @param reg_name The name of the register ("A", "B", or "C"). Case-sensitive.
 * @return The integer value stored in the register.
 * @note Returns 0 and prints an error if the register name is invalid.
 */
int get_register_value(const char* reg_name) {
    if (strcmp(reg_name, "A") == 0) return registers.A;
    if (strcmp(reg_name, "B") == 0) return registers.B;
    if (strcmp(reg_name, "C") == 0) return registers.C;

    fprintf(stderr, "[Runtime Error] Invalid register name specified: '%s'. Expected 'A', 'B', or 'C'.\n", reg_name);
    // Error handling choice: return 0. Could halt.
    return 0;
}

/**
 * @brief Sets the value of a specified register.
 * @param reg_name The name of the register ("A", "B", or "C"). Case-sensitive.
 * @param value The integer value to store in the register.
 * @note Prints an error if the register name is invalid.
 */
void set_register_value(const char* reg_name, int value) {
    if (strcmp(reg_name, "A") == 0) registers.A = value;
    else if (strcmp(reg_name, "B") == 0) registers.B = value;
    else if (strcmp(reg_name, "C") == 0) registers.C = value;
    else {
        fprintf(stderr, "[Runtime Error] Invalid register name specified: '%s'. Expected 'A', 'B', or 'C'.\n", reg_name);
        // Error handling choice: Log error and continue. Could halt.
    }
}

/**
 * @brief Parses an operand string to determine its value and type (immediate or reference).
 * @param operand_str The raw operand string from the instruction.
 * @param is_immediate Pointer to an integer flag. Set to 1 if the operand is immediate (starts with '#'), 0 otherwise.
 * @return The integer value of the operand. If immediate, it's the numeric value. If not immediate,
 *         it attempts to parse as an integer (for addresses/jump targets) but may require further
 *         interpretation by the calling instruction (e.g., as a register name).
 * @note Uses `strtol` for robust conversion of numeric parts, handling potential errors.
 *       Warns if a non-immediate operand doesn't look like a simple integer, as the instruction logic
 *       must handle it (e.g., as a register name).
 */
int parse_operand(const char* operand_str, int* is_immediate) {
    *is_immediate = 0; // Default assumption: operand refers to a register or memory address.
    if (operand_str == NULL) {
        fprintf(stderr, "[Runtime Error] Instruction requires an operand, but none was found.\n");
        return 0; // Error case
    }

    // Check for immediate operand prefix '#'.
    if (operand_str[0] == '#') {
        *is_immediate = 1;
        char* endptr; // Used by strtol to check for conversion errors.
        // Convert the substring after '#' to a long integer (base 10).
        long val = strtol(operand_str + 1, &endptr, 10);

        // Check if strtol consumed the entire relevant part of the string.
        if (*endptr != '\0') {
            fprintf(stderr, "[Runtime Error] Invalid characters in immediate value: '%s'\n", operand_str);
            return 0; // Indicate parsing failure
        }
        // Check for potential overflow/underflow if stricter range checks are needed.
        // if (val > INT_MAX || val < INT_MIN) { /* Handle overflow */ }
        return (int)val;
    }
    else {
        // Not an immediate value. Could be a register name ('A', 'B', 'C'),
        // a memory address (numeric), or a jump target (numeric).
        char* endptr;
        long val = strtol(operand_str, &endptr, 10);

        // If strtol failed to parse anything OR parsed something but left non-whitespace chars,
        // it's likely not a simple integer address/target. Allow register names (alphabetic start).
        // This warning indicates the instruction's logic needs to handle non-numeric operands (like register names).
        if ((*endptr != '\0' && !isspace((unsigned char)*endptr)) && !isalpha((unsigned char)*operand_str)) {
            fprintf(stderr, "[Parser Warning] Operand '%s' is not a simple integer. Instruction logic must interpret it (e.g., as register).\n", operand_str);
            // Return 0 as a placeholder; the instruction must use the original string.
            return 0;
        }
        // If it parsed cleanly as an integer, return the value.
        // Note: Instructions like ADD/SUB/MOV will *ignore* this return value if the operand string
        // is determined to be a register name instead.
        return (int)val;
    }
}


/**
 * @brief Executes a single instruction line.
 * @param line The string containing the instruction and its operands.
 * @param pc The current program counter (line number), used for error reporting.
 * @return The program counter for the *next* instruction to execute.
 *         Returns -1 to signal program halt (HLT instruction or fatal error).
 * @note Parses the instruction line into opcode and operands.
 *       Performs actions based on the opcode (case-insensitive).
 *       Handles operand parsing and register/memory access.
 *       Updates register state and memory.
 *       Determines the next PC value (increment, jump target, or halt).
 */
int run_instruction(const char* line, int pc) {
    if (debug) {
        printf("PC: %02d | Executing: %s\n", pc, line);
    }

    // Create a mutable copy of the instruction line for tokenization.
    char instruction_line[MAX_LINE_LENGTH];
    strncpy(instruction_line, line, MAX_LINE_LENGTH - 1);
    instruction_line[MAX_LINE_LENGTH - 1] = '\0'; // Ensure null termination

    char* token;
    char* parts[4]; // Max expected parts: Opcode, Operand1, Operand2, [Operand3 (rare)]
    int part_count = 0;

    // Tokenize the instruction line. Delimiters include space, tab, and comma.
    // Allows flexible syntax like "ADD A, B" or "ADD A B".
    token = strtok(instruction_line, " \t,");
    while (token != NULL && part_count < 4) {
        parts[part_count++] = token;
        token = strtok(NULL, " \t,"); // Get subsequent tokens
    }

    // If no tokens found (e.g., blank line somehow missed by loader), just advance PC.
    if (part_count == 0) {
        return pc + 1;
    }

    // Extract the opcode (first token).
    char* opcode = parts[0];
    // Convert opcode to uppercase to allow case-insensitive matching.
    for (int i = 0; opcode[i]; i++) {
        opcode[i] = toupper(opcode[i]);
    }

    // --- Instruction Dispatch ---
    // Compare the uppercase opcode with known instructions and execute the corresponding logic.

    // --- Data Movement Instructions ---
    if (strcmp(opcode, "SET") == 0) { // SET <register> <immediate_value>
        if (part_count != 3) { fprintf(stderr, "[Syntax Error] PC %d: SET requires 2 operands (register, immediate_value). Got: %s\n", pc, line); return -1; }
        // Note: SET expects an immediate value directly. Using atoi is simpler here,
        // but strtol could be used for stricter validation if needed.
        int val = atoi(parts[2]);
        set_register_value(parts[1], val);
    }
    else if (strcmp(opcode, "STA") == 0) { // STA <register_or_immediate> <address>
        if (part_count != 3) { fprintf(stderr, "[Syntax Error] PC %d: STA requires 2 operands (register_or_immediate, address). Got: %s\n", pc, line); return -1; }
        int addr = atoi(parts[2]); // Address is expected to be a simple integer.
        int is_immediate;
        // Parse the source operand - could be #value or register name.
        int value_to_store = parse_operand(parts[1], &is_immediate);
        if (is_immediate) {
            write_memory(addr, value_to_store); // Store the immediate value.
        }
        else {
            // If not immediate, assume it's a register name. Get value from register.
            write_memory(addr, get_register_value(parts[1]));
        }
    }
    else if (strcmp(opcode, "LDA") == 0) { // LDA <register> <address>
        if (part_count != 3) { fprintf(stderr, "[Syntax Error] PC %d: LDA requires 2 operands (register, address). Got: %s\n", pc, line); return -1; }
        int addr = atoi(parts[2]); // Address is expected to be a simple integer.
        set_register_value(parts[1], read_memory(addr)); // Load value from memory into register.
    }
    else if (strcmp(opcode, "MOV") == 0) { // MOV <register_dest> <register_src>
        if (part_count != 3) { fprintf(stderr, "[Syntax Error] PC %d: MOV requires 2 operands (register_dest, register_src). Got: %s\n", pc, line); return -1; }
        // Copy value from source register to destination register.
        set_register_value(parts[1], get_register_value(parts[2]));
    }

    // --- Input/Output Instructions ---
    else if (strcmp(opcode, "INP") == 0) { // INP <register>
        if (part_count != 2) { fprintf(stderr, "[Syntax Error] PC %d: INP requires 1 operand (register). Got: %s\n", pc, line); return -1; }
        int input_val;
        printf("INPUT required for register %s: ", parts[1]);
        // Use scanf for input, checking its return value for success.
        if (scanf("%d", &input_val) != 1) {
            fprintf(stderr, "[Runtime Error] Invalid integer input provided.\n");
            // Clear the input buffer to prevent issues with subsequent inputs.
            int c; while ((c = getchar()) != '\n' && c != EOF);
            // Set register to 0 on input error as a default behavior.
            set_register_value(parts[1], 0);
        }
        else {
            set_register_value(parts[1], input_val);
            // Clear the rest of the input line (e.g., the newline character).
            int c; while ((c = getchar()) != '\n' && c != EOF);
        }
    }
    else if (strcmp(opcode, "OUT") == 0) { // OUT <register>
        if (part_count != 2) { fprintf(stderr, "[Syntax Error] PC %d: OUT requires 1 operand (register). Got: %s\n", pc, line); return -1; }
        printf("OUTPUT from register %s: %d\n", parts[1], get_register_value(parts[1]));
    }

    // --- Arithmetic Instructions ---
    else if (strcmp(opcode, "ADD") == 0) { // ADD <register_dest> <register_src>
        if (part_count != 3) { fprintf(stderr, "[Syntax Error] PC %d: ADD requires 2 operands (register_dest, register_src). Got: %s\n", pc, line); return -1; }
        int val1 = get_register_value(parts[1]);
        int val2 = get_register_value(parts[2]);
        // Result stored back in the destination register.
        set_register_value(parts[1], val1 + val2);
        // Note: No overflow checking implemented.
    }
    else if (strcmp(opcode, "SUB") == 0) { // SUB <register_dest> <register_src>
        if (part_count != 3) { fprintf(stderr, "[Syntax Error] PC %d: SUB requires 2 operands (register_dest, register_src). Got: %s\n", pc, line); return -1; }
        int val1 = get_register_value(parts[1]);
        int val2 = get_register_value(parts[2]);
        // Result stored back in the destination register.
        set_register_value(parts[1], val1 - val2);
        // Note: No underflow checking implemented.
    }
    else if (strcmp(opcode, "INC") == 0) { // INC <register>
        if (part_count != 2) { fprintf(stderr, "[Syntax Error] PC %d: INC requires 1 operand (register). Got: %s\n", pc, line); return -1; }
        set_register_value(parts[1], get_register_value(parts[1]) + 1);
    }
    else if (strcmp(opcode, "DEC") == 0) { // DEC <register>
        if (part_count != 2) { fprintf(stderr, "[Syntax Error] PC %d: DEC requires 1 operand (register). Got: %s\n", pc, line); return -1; }
        set_register_value(parts[1], get_register_value(parts[1]) - 1);
    }

    // --- Control Flow Instructions ---
    else if (strcmp(opcode, "JMP") == 0) { // JMP <address>
        if (part_count != 2) { fprintf(stderr, "[Syntax Error] PC %d: JMP requires 1 operand (address). Got: %s\n", pc, line); return -1; }
        // Return the target address directly; this becomes the next PC.
        return atoi(parts[1]);
    }
    else if (strcmp(opcode, "JZ") == 0) { // JZ [register], <address> (Jump if Zero)
        // Supports implicit check on register A (JZ <addr>) or explicit (JZ <reg>, <addr>)
        if (part_count == 2) { // Implicit: JZ <address> (checks register A)
            if (registers.A == 0) return atoi(parts[1]); // Jump if A is zero
        }
        else if (part_count == 3) { // Explicit: JZ <register>, <address>
            if (get_register_value(parts[1]) == 0) return atoi(parts[2]); // Jump if specified register is zero
        }
        else {
            fprintf(stderr, "[Syntax Error] PC %d: JZ requires 1 or 2 operands ([register], address). Got: %s\n", pc, line); return -1;
        }
        // If condition is not met, execution falls through to the next instruction.
    }
    else if (strcmp(opcode, "JNZ") == 0) { // JNZ [register], <address> (Jump if Not Zero)
        if (part_count == 2) { // Implicit: JNZ <address> (checks register A)
            if (registers.A != 0) return atoi(parts[1]); // Jump if A is not zero
        }
        else if (part_count == 3) { // Explicit: JNZ <register>, <address>
            if (get_register_value(parts[1]) != 0) return atoi(parts[2]); // Jump if specified register is not zero
        }
        else {
            fprintf(stderr, "[Syntax Error] PC %d: JNZ requires 1 or 2 operands ([register], address). Got: %s\n", pc, line); return -1;
        }
        // Fall through if condition not met.
    }
    else if (strcmp(opcode, "JP") == 0) { // JP [register], <address> (Jump if Positive)
        if (part_count == 2) { // Implicit: JP <address> (checks register A)
            if (registers.A > 0) return atoi(parts[1]); // Jump if A is positive
        }
        else if (part_count == 3) { // Explicit: JP <register>, <address>
            if (get_register_value(parts[1]) > 0) return atoi(parts[2]); // Jump if specified register is positive
        }
        else {
            fprintf(stderr, "[Syntax Error] PC %d: JP requires 1 or 2 operands ([register], address). Got: %s\n", pc, line); return -1;
        }
        // Fall through if condition not met.
    }
    else if (strcmp(opcode, "JN") == 0) { // JN [register], <address> (Jump if Negative)
        if (part_count == 2) { // Implicit: JN <address> (checks register A)
            if (registers.A < 0) return atoi(parts[1]); // Jump if A is negative
        }
        else if (part_count == 3) { // Explicit: JN <register>, <address>
            if (get_register_value(parts[1]) < 0) return atoi(parts[2]); // Jump if specified register is negative
        }
        else {
            fprintf(stderr, "[Syntax Error] PC %d: JN requires 1 or 2 operands ([register], address). Got: %s\n", pc, line); return -1;
        }
        // Fall through if condition not met.
    }

    // --- Utility/Control Instructions ---
    else if (strcmp(opcode, "CLRR") == 0) { // CLRR [register] (Clear Register(s))
        if (part_count == 1) { // No operand: Clear all registers A, B, C
            registers.A = 0;
            registers.B = 0;
            registers.C = 0;
        }
        else if (part_count == 2) { // One operand: Clear specified register
            set_register_value(parts[1], 0);
        }
        else {
            fprintf(stderr, "[Syntax Error] PC %d: CLRR requires 0 or 1 operands ([register]). Got: %s\n", pc, line); return -1;
        }
    }
    else if (strcmp(opcode, "CLRM") == 0) { // CLRM [address] (Clear Memory)
        if (part_count == 1) { // No operand: Clear all of main memory
            // Use memset for efficient zeroing of the entire memory array.
            memset(memory, 0, sizeof(memory));
        }
        else if (part_count == 2) { // One operand: Clear specific memory address
            int addr = atoi(parts[1]);
            write_memory(addr, 0);
        }
        else {
            fprintf(stderr, "[Syntax Error] PC %d: CLRM requires 0 or 1 operands ([address]). Got: %s\n", pc, line); return -1;
        }
    }
    else if (strcmp(opcode, "DMP") == 0) { // DMP (Dump state)
        if (part_count != 1) { fprintf(stderr, "[Syntax Error] PC %d: DMP requires no operands. Got: %s\n", pc, line); return -1; }
        dump_contents(); // Call the state dump function.
    }
    else if (strcmp(opcode, "HLT") == 0) { // HLT (Halt execution)
        if (part_count != 1) { fprintf(stderr, "[Syntax Error] PC %d: HLT requires no operands. Got: %s\n", pc, line); return -1; }
        printf("--- Program Halted by HLT instruction at PC %d ---\n", pc);
        return -1; // Signal halt to the main execution loop.
    }

    // --- Error Handling for Unknown Instructions ---
    else {
        fprintf(stderr, "[Runtime Error] Unknown instruction encountered at PC %d: '%s'\n", pc, opcode);
        return -1; // Halt execution on unrecognized instruction.
    }

    // Default behavior for most instructions: advance to the next line.
    return pc + 1;
}

/**
 * @brief Manages the main fetch-decode-execute cycle of the simulator.
 *
 * Initializes the machine state (memory), then iteratively fetches instructions
 * from program memory using the program counter (PC), executes them via
 * `run_instruction`, and updates the PC until a halt condition (-1) is returned
 * or the PC goes out of bounds.
 */
void run_program() {
    int pc = 0; // Initialize Program Counter to the start of the program (line 0).

    // Ensure memory starts in a known state (all zeros).
    memset(memory, 0, sizeof(memory));

    // Main execution loop: continues as long as PC is valid and no halt signal received.
    while (pc >= 0 && pc < program_line_count) {
        // Execute the instruction at the current PC and get the *next* PC value.
        int next_pc = run_instruction(program_memory[pc], pc);

        // Check for halt signal (-1) from run_instruction.
        if (next_pc == -1) {
            break; // Exit the loop if HLT or a fatal error occurred.
        }

        // Update the Program Counter for the next iteration.
        pc = next_pc;

        // Optional: Display brief state update after each instruction in debug mode.
        if (debug) {
            printf("  State after instruction: A=%d B=%d C=%d\n", registers.A, registers.B, registers.C);
            // Could add memory dump here if needed for step-by-step debugging.
        }
    }

    // --- Post-Execution Summary ---
    // Check why the loop terminated.
    if (pc >= program_line_count) {
        printf("--- Program execution finished: Reached end of loaded instructions (PC=%d) ---\n", pc);
    }
    else if (pc < -1) {
        // Note: Current implementation uses only -1 for halt/error.
        // This branch allows for future distinct error codes if needed.
        fprintf(stderr, "--- Program Terminated Abnormally (PC=%d indicates error state) ---\n", pc);
    }
    // If pc == -1, the HLT message or error message was already printed by run_instruction.
}

/**
 * @brief Dumps the current state of the CPU registers and main memory to standard output.
 *
 * Useful for debugging. Prints register values and formats memory output
 * for readability.
 */
void dump_contents() {
    printf("\n--- CPU State Dump ---\n");
    printf("Registers: A=%-5d B=%-5d C=%-5d\n", registers.A, registers.B, registers.C);
    printf("Memory Contents (%d words):\n", MEMORY_SIZE);
    // Print memory, 8 words per line, with address labels.
    for (int i = 0; i < MEMORY_SIZE; ++i) {
        if (i % 8 == 0) {
            printf("  [%02d]:", i); // Address label at the start of the row.
        }
        printf(" %5d", memory[i]); // Print value, padded for alignment.
        if ((i + 1) % 8 == 0 || i == MEMORY_SIZE - 1) {
            printf("\n"); // Newline at the end of the row or end of memory.
        }
    }
    printf("----------------------\n");
}

/**
 * @brief Main entry point of the simulator.
 *
 * Handles command-line interaction to get the program filename and debug setting.
 * Orchestrates the loading and execution of the specified program.
 * Provides final status messages and optionally dumps final state if debug is enabled.
 * @return 0 on successful completion, 1 on critical errors (e.g., file loading failure).
 */
int main() {
    char program_filename[MAX_FILENAME_LENGTH];
    char debug_input[10]; // Small buffer for "yes"/"no" type input

    // --- User Input: Program File ---
    printf("Enter the program filename (e.g., program.txt): ");
    // Use fgets for safer input handling than scanf; prevents buffer overflows.
    if (fgets(program_filename, sizeof(program_filename), stdin) == NULL) {
        fprintf(stderr, "[Fatal Error] Failed to read filename from input.\n");
        return 1;
    }
    // Remove the trailing newline character read by fgets.
    program_filename[strcspn(program_filename, "\r\n")] = 0;

    // --- User Input: Debug Mode ---
    printf("Enable debug mode? (yes/no or 1/0): ");
    if (fgets(debug_input, sizeof(debug_input), stdin) == NULL) {
        fprintf(stderr, "[Fatal Error] Failed to read debug setting from input.\n");
        return 1;
    }
    // Remove the trailing newline character.
    debug_input[strcspn(debug_input, "\r\n")] = 0;

    // --- Configure Debug Flag ---
    // Check common affirmative inputs for enabling debug mode (case-insensitive check not implemented here for simplicity).
    if (strcmp(debug_input, "yes") == 0 || strcmp(debug_input, "y") == 0 ||
        strcmp(debug_input, "1") == 0 || strcmp(debug_input, "true") == 0) {
        debug = 1;
        printf("Debug mode enabled. Verbose output will be shown.\n");
    }
    else {
        debug = 0;
        printf("Debug mode disabled.\n");
    }

    // --- Load Program ---
    printf("Loading program from '%s'...\n", program_filename);
    if (load_program(program_filename) < 0) {
        // Error message already printed by load_program (via perror).
        fprintf(stderr, "[Fatal Error] Program loading failed. Exiting.\n");
        return 1; // Exit if the program could not be loaded.
    }

    // --- Execute Program ---
    printf("Starting program execution...\n");
    run_program(); // Enter the main execution cycle.

    // --- Final State Dump (Optional) ---
    // If debug mode was enabled, dump the final state of the machine.
    if (debug) {
        printf("Execution finished. Dumping final state:\n");
        dump_contents();
    }

    printf("Simulator finished.\n");
    return 0; // Indicate successful completion.
}