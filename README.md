# CPUsim

A project consisting of two C programs: an assembler and a simulator for a custom 16-bit CPU architecture.

*   `assembler.exe`: Compiles human-readable assembly language into a binary machine code format.
*   `simulator.exe`: Loads and executes the binary files, simulating the CPU's behavior and running the compiled program.

## Core Concepts

*   **Registers:** The CPU has three general-purpose integer registers: `A`, `B`, `C`.
*   **Memory:** Main memory consists of `MEMORY_SIZE` (default 64) integer words, addressed from `0` to `MEMORY_SIZE - 1`.
*   **Immediate Values:** Literal numeric values. In this instruction set, immediate values must be prefixed with a hash symbol (`#`). For example: `#42`.
*   **Labels:** You can mark a line of code with a label by writing a name followed by a colon (e.g., `my_loop:`). Labels provide a convenient way to refer to an address for jump instructions, which is much easier than manually counting lines.
*   **Comments:** Lines starting with a semicolon (`;`) are treated as comments and are ignored by the assembler.
*   **Operands:** Operands are separated by spaces or commas (e.g., `ADD A, B` and `ADD A B` are both valid).

---

## Instruction Set Reference

### Data Movement Instructions

1.  **`SET <register> #<value>`**
    *   **Syntax:** `SET A, #100`
    *   **Description:** Sets the specified `<register>` to the integer `<value>`.
    *   **Operands:**
        *   `<register>`: The destination register (A, B, or C).
        *   `#<value>`: An integer value, prefixed with `#`.

2.  **`STA <register> <address>`**
    *   **Syntax:** `STA A, 5`
    *   **Description:** Stores the value from the source `<register>` into the memory location specified by `<address>`.
    *   **Operands:**
        *   `<register>`: The source register (A, B, C) whose value will be stored.
        *   `<address>`: The memory address (0 to MEMORY\_SIZE-1) or a label.

3.  **`LDA <register> <address>`**
    *   **Syntax:** `LDA B, 5`
    *   **Description:** Loads the value from the memory location specified by `<address>` into the `<register>`.
    *   **Operands:**
        *   `<register>`: The destination register (A, B, or C).
        *   `<address>`: The memory address (0 to MEMORY\_SIZE-1) or a label to read from.

4.  **`MOV <reg_dest> <reg_src>`**
    *   **Syntax:** `MOV A, B`
    *   **Description:** Copies the value from `<reg_src>` to `<reg_dest>`.
    *   **Operands:**
        *   `<reg_dest>`: The destination register (A, B, C).
        *   `<reg_src>`: The source register (A, B, C).

### Input/Output Instructions

5.  **`INP <register>`**
    *   **Syntax:** `INP A`
    *   **Description:** Prompts the user for an integer input and stores it in the specified `<register>`.
    *   **Operands:**
        *   `<register>`: The register (A, B, or C) to store the input.

6.  **`OUT <register>`**
    *   **Syntax:** `OUT A`
    *   **Description:** Prints the integer value of the specified `<register>` to the console.
    *   **Operands:**
        *   `<register>`: The register (A, B, or C) whose value is to be printed.

### Arithmetic Instructions

7.  **`ADD <reg_dest> <reg_src>`**
    *   **Syntax:** `ADD A, B`
    *   **Description:** Adds the value of `<reg_src>` to `<reg_dest>` and stores the result in `<reg_dest>`.
    *   **Operands:**
        *   `<reg_dest>`: The register that acts as the first operand and destination.
        *   `<reg_src>`: The register whose value is added.

8.  **`SUB <reg_dest> <reg_src>`**
    *   **Syntax:** `SUB A, B`
    *   **Description:** Subtracts the value of `<reg_src>` from `<reg_dest>` and stores the result in `<reg_dest>`.
    *   **Operands:**
        *   `<reg_dest>`: The register to be subtracted from and store the result.
        *   `<reg_src>`: The register whose value is subtracted.

9.  **`INC <register>`**
    *   **Syntax:** `INC A`
    *   **Description:** Increments the value of the specified `<register>` by 1.
    *   **Operands:**
        *   `<register>`: The register (A, B, or C) to increment.

10. **`DEC <register>`**
    *   **Syntax:** `DEC A`
    *   **Description:** Decrements the value of the specified `<register>` by 1.
    *   **Operands:**
        *   `<register>`: The register (A, B, or C) to decrement.

### Control Flow Instructions

11. **`JMP <address_or_label>`**
    *   **Syntax:** `JMP 5` or `JMP my_loop`
    *   **Description:** Unconditionally jumps execution to the specified address or label.
    *   **Operands:**
        *   `<address_or_label>`: A 0-indexed line number or a defined label.

12. **`JZ <register>, <address_or_label>`**
    *   **Syntax:** `JZ A, 10` or `JZ B, end_loop`
    *   **Description:** Jumps to the specified address or label if the value of the `<register>` is zero.
    *   **Operands:**
        *   `<register>`: The register (A, B, C) to check. **This is not optional.**
        *   `<address_or_label>`: The target to jump to if the condition is true.

13. **`JNZ <register>, <address_or_label>`**
    *   **Syntax:** `JNZ C, countdown`
    *   **Description:** Jumps if the value of the `<register>` is **not** zero.
    *   **Operands:**
        *   `<register>`: The register (A, B, C) to check. **This is not optional.**
        *   `<address_or_label>`: The target to jump to.

14. **`JP <register>, <address_or_label>`**
    *   **Syntax:** `JP A, is_positive`
    *   **Description:** Jumps if the value of the `<register>` is positive (> 0).
    *   **Operands:**
        *   `<register>`: The register (A, B, C) to check. **This is not optional.**
        *   `<address_or_label>`: The target to jump to.

15. **`JN <register>, <address_or_label>`**
    *   **Syntax:** `JN B, is_negative`
    *   **Description:** Jumps if the value of the `<register>` is negative (< 0).
    *   **Operands:**
        *   `<register>`: The register (A, B, C) to check. **This is not optional.**
        *   `<address_or_label>`: The target to jump to.

### Utility/Control Instructions

16. **`CLRR`**
    *   **Syntax:** `CLRR`
    *   **Description:** Clears **all** general-purpose registers (A, B, and C) to zero. Takes no operands.
    *   **Operands:** None.

17. **`CLRM`**
    *   **Syntax:** `CLRM`
    *   **Description:** Clears **all** of main memory to zero. Takes no operands.
    *   **Operands:** None.

18. **`DMP`**
    *   **Syntax:** `DMP`
    *   **Description:** Dumps the current state of all CPU registers and main memory to the console.
    *   **Operands:** None.

19. **`HLT`**
    *   **Syntax:** `HLT`
    *   **Description:** Halts program execution.
    *   **Operands:** None.

---

## Example Program (`program.txt`)

This example demonstrates correct syntax, comments, and the use of labels.

```assembly
; --- Example Program for the CPUsim Toolchain ---
; This program takes two numbers, adds them, performs a countdown,
; and demonstrates conditional jumps.

; Get user input and add two numbers
INP A
INP B
ADD A, B
OUT A

; Store the result in memory at address 10, then load it into C and print
STA A, 10
LDA C, 10
OUT C

; Demonstrate setting an immediate value and storing it
SET A, #123
STA A, 11
LDA B, 11
OUT B

; Demonstrate simple arithmetic
SET A, #20
MOV B, A
INC B
DEC A
SUB B, A
OUT B

; --- Countdown Loop using a Label ---
SET C, #3       ; Initialize counter outside the loop

countdown_loop:
  OUT C           ; Print the current counter value
  DEC C           ; Decrement the counter
  JNZ C, countdown_loop  ; If C is not zero, jump back to the label

; After the loop finishes, C is 0. This line will print 0.
OUT C

; --- Conditional Jump Demo ---
SET A, #0
SET B, #77

; Since A is zero, this jump will be taken, skipping the next two OUT instructions
JZ A, skip_print

; These lines are skipped
OUT B
OUT A

skip_print:
  CLRM            ; Clear all of memory
  DMP             ; Dump the final state of registers and memory
  HLT             ; End of program
```