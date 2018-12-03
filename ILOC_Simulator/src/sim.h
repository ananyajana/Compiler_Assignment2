/*  sim.h
 *  Header file for a simulator of the ILOC subset defined in
 *  "Engineering a Compiler" by Cooper and Torczon
 *  written by Todd Waterman
 *  11/30/00 */

#ifndef _SIM_H_
#define _SIM_H_

/* These flags determine what the simulator stalls on */
int stall_on_branches;
int stall_on_memory;
int stall_on_registers;

typedef enum effect_type {REGISTER=0,MEMORY,BRANCH,DISPLAY} Effect_Type; 

/* This keeps track of assignments to registers or memory so 
   parallel operations can be simulated */
typedef struct change {
    Effect_Type type;
    int location;
    int value;
    Instruction* target;
    int cycles_away;
    struct change* next;
} Change;

/* Print a usage message */
void print_help();

/* Set stall flags */
void set_stall_mode(int);

/* Simulate the code and output results to standard out */
void simulate(Instruction* code);

/* Returns 1 if the instruction uses a register that is not ready */
int register_stall(Instruction*,Change*);

/* Returns 1 if all operands in the list are ready, and 
   return 0 if they are not */
int list_of_operands_ready(Operand* reg, Change*);
 
/* Returns 1 if the instruction uses a memory address that is not ready */
int memory_stall(Instruction*,Change*);

/* Returns 1 if the register does not depend on the list of effects */
int reg_ready(int register, Change* effects);

/* Returns 1 if the memory location does not depend on the list of effects */
int mem_ready(int location, Change* effects);

/* Returns 1 if the word of memory does not depend on the list of effects */
int word_ready(int location, Change* effects);

/* Execute all operations in a single instruction */
Change* execute_instruction(Instruction* inst, int* op_count, int* vect_count);

/* Execute a single operation */
Change* execute_operation(Operation* op);

/* onereg creates most of a change structure for the common case where 
   a single register is defined. */
Change* onereg(Operation* op);

/* storeop creates most of a change structure for a store operation */
Change* storeop(Operation* op);

/* Returns 1 if there is an outstanding branch instruction */
int branch_stall(Change*);

/* Reduces the cycles_away of all changes by one and executes any changes
   that have a cycles_away of 0 */
Change* execute_changes(Change*,Change**,Instruction**);

/* Returns false if machine constraints are violated. */
int check_machine_constraints(Instruction*);

#endif /* _SIM_H_ */

