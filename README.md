# CPUsim

A project consisting of two C programs: an assembler and a simulator for a custom 16-bit CPU architecture.

*   `assembler.exe`: Compiles human-readable assembly language into a binary machine code format.
*   `simulator.exe`: Loads and executes the binary files, simulating the CPU's behavior and running the compiled program.

Example usage:

```bash
gcc assembler.c -o assembler
gcc simulator.c -o simulator
./assembler program.txt program.bin
./simulator program.bin
```

## Core

*   **Registers:** The CPU has 8 general-purpose registers: `EAX`, `EBX`, `ECX`, `EDX`, `ESI`, `EDI`, `EBP`, `ESP`.
*   **Memory:** Main memory consists of `MEMORY_SIZE` (default 256) integer words.
*   **Stack:** The stack grows downwards from the top of memory. `ESP` is the stack pointer and `EBP` is the base pointer.
*   **Flags:** The CPU has a Zero Flag (`ZF`) and a Sign Flag (`SF`) which are set by comparison instructions.
*   **Immediate Values:** Literal numeric values, prefixed with a hash symbol (`#`). Example: `#42`.
*   **Labels:** Mark a line of code with a label by writing a name followed by a colon (e.g., `my_loop:`).
*   **Comments:** Lines starting with a semicolon (`;`) are comments.
*   **Operands:** Operands can be separated by spaces or commas.

---

## Instruction Set Reference

### Data Movement Instructions

1.  **`MOV <dest>, <src>`**
    *   **Description:** Copies data from a source to a destination. This is a versatile instruction with multiple forms.
    *   **Forms:**
        *   `MOV <reg>, #<imm>`: Move immediate value to register. (e.g., `MOV EAX, #100`)
        *   `MOV <reg>, <reg>`: Copy value from one register to another. (e.g., `MOV EAX, EBX`)
        *   `MOV <reg>, [<addr>]`: Load value from a memory address to a register. (e.g., `MOV EAX, [10]`)
        *   `MOV [<addr>], <reg>`: Store value from a register to a memory address. (e.g., `MOV [10], EAX`)
        *   `MOV <reg>, [<reg>+<offset>]`: Load with base+offset addressing. (e.g., `MOV EAX, [EBP+4]`)
        *   `MOV [<reg>+<offset>], <reg>`: Store with base+offset addressing. (e.g., `MOV [EBP-8], ECX`)

### Input/Output Instructions

2.  **`INP <register>`**
    *   **Description:** Prompts for user input and stores it in a register. (e.g., `INP EAX`)

3.  **`OUT <register>`**
    *   **Description:** Prints the value of a register to the console. (e.g., `OUT EAX`)

### Arithmetic Instructions

4.  **`ADD <reg>, <reg|#imm>`**
    *   **Description:** Adds the second operand to the first, storing the result in the first.
    *   **Forms:**
        *   `ADD <reg>, <reg>` (e.g., `ADD EAX, EBX`)
        *   `ADD <reg>, #<imm>` (e.g., `ADD ECX, #5`)

5.  **`SUB <reg>, <reg|#imm>`**
    *   **Description:** Subtracts the second operand from the first, storing the result in the first.
    *   **Forms:**
        *   `SUB <reg>, <reg>` (e.g., `SUB EAX, EBX`)
        *   `SUB <reg>, #<imm>` (e.g., `SUB ECX, #5`)

6.  **`MUL <reg>, <reg>`**
    *   **Description:** Multiplies two registers, storing the result in the first. (e.g., `MUL EAX, EBX`)

7.  **`DIV <reg>, <reg>`**
    *   **Description:** Divides the first register by the second, storing the result in the first. (e.g., `DIV EAX, EBX`)

8.  **`INC <register>`**
    *   **Description:** Increments a register by 1. (e.g., `INC EAX`)

9.  **`DEC <register>`**
    *   **Description:** Decrements a register by 1. (e.g., `DEC EAX`)

### Logical Instructions

10. **`XOR <reg>, <reg>`**
    *   **Description:** Performs a bitwise XOR, storing the result in the first register. (e.g., `XOR EAX, EBX`)

11. **`NOT <register>`**
    *   **Description:** Performs a bitwise NOT on the register. (e.g., `NOT EAX`)

### Comparison

12. **`CMP <reg>, <reg|#imm>`**
    *   **Description:** Compares two operands and sets the `ZF` and `SF` flags. Does not modify operands.
    *   **Forms:**
        *   `CMP <reg>, <reg>` (e.g., `CMP EAX, EBX`)
        *   `CMP <reg>, #<imm>` (e.g., `CMP ECX, #10`)

### Control Flow Instructions

13. **`JMP <address_or_label>`**
    *   **Description:** Unconditionally jumps to a target address or label. (e.g., `JMP my_loop`)

14. **Conditional Jumps**
    *   **Description:** Jumps to a target if a condition based on the flags is met.
    *   **Instructions:**
        *   `JE` / `JZ`: Jump if Equal / Jump if Zero (`ZF=1`)
        *   `JNE` / `JNZ`: Jump if Not Equal / Jump if Not Zero (`ZF=0`)
        *   `JG` / `JNLE`: Jump if Greater (`ZF=0` and `SF=0`)
        *   `JL` / `JNGE`: Jump if Less (`SF=1`)
        *   `JGE` / `JNL`: Jump if Greater or Equal (`SF=0`)
        *   `JLE` / `JNG`: Jump if Less or Equal (`ZF=1` or `SF=1`)

### Stack & Function Instructions

15. **`PUSH <register>`**
    *   **Description:** Pushes a register's value onto the stack. (e.g., `PUSH EAX`)

16. **`POP <register>`**
    *   **Description:** Pops a value from the stack into a register. (e.g., `POP EAX`)

17. **`CALL <address_or_label>`**
    *   **Description:** Pushes the return address onto the stack and jumps to a function. (e.g., `CALL my_function`)

18. **`RET`**
    *   **Description:** Pops the return address from the stack and jumps to it.

### System Instructions

19. **`HLT`**
    *   **Description:** Halts program execution.

---

## Example Program (`program.txt`)

This example demonstrates the architecture.

```assembly
; --- Example Program for the CPUsim Toolchain ---
; This program calculates the factorial of a number using a recursive function.

; --- Main Program ---
main:
  MOV EAX, #5          ; Set the number to calculate the factorial of (e.g., 5)
  PUSH EAX             ; Push the argument onto the stack
  CALL factorial       ; Call the factorial function
  POP EAX              ; Clean up the stack (the argument)
  OUT EAX              ; Print the result
  HLT                  ; Halt the program

; --- Factorial Function ---
; Calculates the factorial of the number passed on the stack.
; Argument: n (at [EBP+2])
; Returns: factorial(n) in EAX
factorial:
  ; --- Function Prologue ---
  PUSH EBP             ; Save the old base pointer
  MOV EBP, ESP         ; Set up the new stack frame

  ; --- Base Case ---
  MOV EAX, [EBP+2]     ; Get the argument n
  CMP EAX, #1          ; Compare n with 1
  JLE end_factorial    ; If n <= 1, jump to the end (return 1)

  ; --- Recursive Step ---
  DEC EAX              ; n - 1
  PUSH EAX             ; Push (n - 1) as the argument for the recursive call
  CALL factorial       ; factorial(n - 1)
  POP EBX              ; Clean up the stack (the argument)

  MOV EBX, [EBP+2]     ; Get the original n again
  MUL EAX, EBX         ; result = n * factorial(n - 1)

end_factorial:
  ; --- Function Epilogue ---
  MOV ESP, EBP         ; Restore the stack pointer
  POP EBP              ; Restore the old base pointer
  RET                  ; Return to the caller
```
