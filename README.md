# CPUsim
C program to simulate a basic CPU &amp; instruction set

*   **Registers:** The CPU has three general-purpose integer registers: `A`, `B`, `C`.
*   **Memory:** Main memory consists of `MEMORY_SIZE` (default 64) integer words, addressed from `0` to `MEMORY_SIZE - 1`.
*   **Immediate Values:** Numeric values directly embedded in an instruction. For instructions like `STA` that can take either a register or an immediate value as a source, immediate values must be prefixed with `#` (e.g., `#42`). For `SET`, the `#` is implied for the value.
*   **Line Numbers:** Jump instructions (`JMP`, `JZ`, etc.) refer to 0-indexed line numbers in the program file.
*   **Comments:** Lines starting with a semicolon (`;`) are ignored.
*   **Operands:** Operands are typically separated by spaces or commas.

---

### Data Movement Instructions

1.  **`SET <register> <value>`**
    *   **Syntax:** `SET A 100`
    *   **Description:** Sets the specified `<register>` to the integer `<value>`.
    *   **Operands:**
        *   `<register>`: The destination register (A, B, or C).
        *   `<value>`: An integer.

2.  **`STA <source> <address>`**
    *   **Syntax:** `STA A 5` or `STA #50 6`
    *   **Description:** Stores the value from `<source>` into the memory location specified by `<address>`.
    *   **Operands:**
        *   `<source>`: Can be a register (A, B, C) or an immediate value (e.g., `#42`).
        *   `<address>`: The memory address (0 to MEMORY\_SIZE-1) to write to.

3.  **`LDA <register> <address>`**
    *   **Syntax:** `LDA B 5`
    *   **Description:** Loads the value from the memory location specified by `<address>` into the `<register>`.
    *   **Operands:**
        *   `<register>`: The destination register (A, B, or C).
        *   `<address>`: The memory address (0 to MEMORY\_SIZE-1) to read from.

4.  **`MOV <reg_dest> <reg_src>`**
    *   **Syntax:** `MOV A B`
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
    *   **Syntax:** `ADD A B`
    *   **Description:** Adds the value of `<reg_src>` to `<reg_dest>` and stores the result in `<reg_dest>`.
    *   **Operands:**
        *   `<reg_dest>`: The register (A, B, C) to add to and store the result.
        *   `<reg_src>`: The register (A, B, C) whose value is added.

8.  **`SUB <reg_dest> <reg_src>`**
    *   **Syntax:** `SUB A B`
    *   **Description:** Subtracts the value of `<reg_src>` from `<reg_dest>` and stores the result in `<reg_dest>`.
    *   **Operands:**
        *   `<reg_dest>`: The register (A, B, C) to subtract from and store the result.
        *   `<reg_src>`: The register (A, B, C) whose value is subtracted.

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

11. **`JMP <address>`**
    *   **Syntax:** `JMP 5`
    *   **Description:** Unconditionally jumps execution to the program line number specified by `<address>`.
    *   **Operands:**
        *   `<address>`: The 0-indexed line number to jump to.

12. **`JZ [register], <address>`**
    *   **Syntax:** `JZ 10` or `JZ B 10`
    *   **Description:** Jumps to `<address>` if the value of the specified `[register]` (or register `A` if no register is specified) is zero.
    *   **Operands:**
        *   `[register]` (optional): The register (A, B, C) to check. Defaults to `A`.
        *   `<address>`: The 0-indexed line number to jump to if the condition is true.

13. **`JNZ [register], <address>`**
    *   **Syntax:** `JNZ 10` or `JNZ C 10`
    *   **Description:** Jumps to `<address>` if the value of the specified `[register]` (or register `A` if no register is specified) is not zero.
    *   **Operands:**
        *   `[register]` (optional): The register (A, B, C) to check. Defaults to `A`.
        *   `<address>`: The 0-indexed line number to jump to if the condition is true.

14. **`JP [register], <address>`**
    *   **Syntax:** `JP 10` or `JP A 10`
    *   **Description:** Jumps to `<address>` if the value of the specified `[register]` (or register `A` if no register is specified) is positive (> 0).
    *   **Operands:**
        *   `[register]` (optional): The register (A, B, C) to check. Defaults to `A`.
        *   `<address>`: The 0-indexed line number to jump to if the condition is true.

15. **`JN [register], <address>`**
    *   **Syntax:** `JN 10` or `JN B 10`
    *   **Description:** Jumps to `<address>` if the value of the specified `[register]` (or register `A` if no register is specified) is negative (< 0).
    *   **Operands:**
        *   `[register]` (optional): The register (A, B, C) to check. Defaults to `A`.
        *   `<address>`: The 0-indexed line number to jump to if the condition is true.

### Utility/Control Instructions

16. **`CLRR [register]`**
    *   **Syntax:** `CLRR` or `CLRR B`
    *   **Description:** Clears register(s) to zero. If no `<register>` is specified, clears all registers (A, B, C). Otherwise, clears only the specified `<register>`.
    *   **Operands:**
        *   `[register]` (optional): The specific register (A, B, C) to clear.

17. **`CLRM [address]`**
    *   **Syntax:** `CLRM` or `CLRM 15`
    *   **Description:** Clears memory location(s) to zero. If no `<address>` is specified, clears all of main memory. Otherwise, clears only the specified memory `<address>`.
    *   **Operands:**
        *   `[address]` (optional): The specific memory address (0 to MEMORY\_SIZE-1) to clear.

18. **`DMP`**
    *   **Syntax:** `DMP`
    *   **Description:** Dumps the current state of all CPU registers (A, B, C) and the entire main memory contents to the console. Useful for debugging.
    *   **Operands:** None.

19. **`HLT`**
    *   **Syntax:** `HLT`
    *   **Description:** Halts program execution.
    *   **Operands:** None.

---

Example program.txt
```
INP A
INP B
ADD A B
OUT A
STA A 10
CLRR C
LDA C 10
OUT C
STA #123 11
LDA B 11
OUT B
SET A 20
MOV B A
INC B
DEC A
SUB B A
OUT B
SET C 3
OUT C
DEC C
JNZ C 18
OUT C
SET A 0
SET B 77
JZ A 26
OUT B
OUT A
CLRM
DMP
HLT
```
