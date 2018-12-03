/*  sim.c
 *  Source code for a simulator of the ILOC subset defined in
 *  "Engineering a Compiler" by Cooper and Torczon
 *  written by Todd Waterman
 *  11/30/00 
 *
 * modified by Chung-Hsing Hsu at Rutgers University
 *  set_stall_mode(3) instead of 2
 *  all latencies of one, except two for loads/stores
 +
 + modified by Uli Kremer
 +  added include <string.h>   09/27/04
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "instruction.h"
#include "machine.h"
#include "sim.h"

int vect_on = 0; /* Signals that operations can be vectorized; set by VECTON and VECTOFF instructions. Uli  04/13/2011 */

int main(int argc, char* argv[])
{
    int mem_size = 0;
    int reg_size = 0;
    int current_argument = 1;
    int machine_initialized = 0;
    Instruction* code;

    /* Set default stall mode - branches, registers, and memory */
    set_stall_mode(3 /*2*/);

    while(current_argument < argc) 
    {
	if (strcmp(argv[current_argument],"-h") == 0)
	{
	    print_help();
	    return 0;
	}
	
	/* All of the following flags require at least one additional parameter,
	   so perform check here. */
	if (current_argument == argc - 1)
	{
	    fprintf(stderr,"Invalid flag sequence: make sure any required numbers are included.\n");
	    return(1);
	}


	if (strcmp(argv[current_argument],"-r") == 0)
	{
	    reg_size = (int) strtol(argv[current_argument+1],(char**)NULL,10);
	    current_argument += 2;
	}
	else
	{
	    if (strcmp(argv[current_argument],"-m") == 0)
	    {
		mem_size = (int) strtol(argv[current_argument+1],(char**)NULL,10);
		current_argument += 2;
	    }
	    else
	    {
		if (strcmp(argv[current_argument],"-s") == 0)
		{
		    set_stall_mode((int)strtol(argv[current_argument+1],(char**)NULL,10));
		    current_argument += 2;
		}
		else
		{
		    if (strcmp(argv[current_argument],"-i") == 0)
		    {
			int i;
			int start_location = (int)strtol(argv[current_argument+1],(char**)NULL,10);
			initialize_machine(reg_size,mem_size);
			machine_initialized = 1;
			for(i = 0; i < (argc - current_argument - 2); i++)
			    set_word(start_location + 4*i, (int)strtol(argv[current_argument+2+i],
								       (char**)NULL,10));
			current_argument = argc;
		    }
		    else
		    {
			if (strcmp(argv[current_argument],"-c") == 0)
			{
			    int i;
			    int start_location = (int)strtol(argv[current_argument+1],
							     (char**)NULL,10);
			    initialize_machine(reg_size,mem_size);
			    machine_initialized = 1;
			    for(i = 0; i < (argc - current_argument - 2); i++)
				set_memory(start_location + i, 
					 (char)strtol(argv[current_argument+2+i],
						     (char**)NULL,10));
			    current_argument = argc;
			}
			else
			{
			    fprintf(stderr,"Invalid flag specified\n");
			    return 0;
			}
		    }
		}
	    }
	}
	
    }
    
    if (!machine_initialized)
	initialize_machine(reg_size,mem_size);

    code = parse();

    if (!code)
    {
	fprintf(stderr,"Error reading input file, simulator not run.\n");
	return 1;
    }

    simulate(code);

    return 0;
};

/* Print a usage message */
void print_help()
{
    printf("Usage: sim [options] < filename\n");
    printf("  Options:\n");
    printf("    -h                 display usage message\n");
    printf("    -r NUM             simulator has NUM available registers\n");
    printf("    -m NUM             simulator has NUM bytes of memory\n");
    printf("    -s NUM             simulator stalls for the following conditions:\n");
    printf("                         0:  nothing\n");
    printf("                         1:  branches\n");
    printf("                         2:  branches and memory interlocks\n");
    printf("                         3:  branches and both register and memory interlocks\n");
    printf("    -i NUM ... NUM     starting at the memory location specified by the first\n");
    printf("                         NUM put the remaining NUMs into memory as words.\n");
    printf("                         Must be the last option specified\n");
    printf("    -c NUM ... NUM     same as -i, but puts the NUMs into memory as bytes\n");
    printf("  filename should be a valid ILOC input file\n");

}

/* Set stall flags appropriately */
void set_stall_mode(int mode)
{
    stall_on_branches = 0;
    stall_on_memory = 0;
    stall_on_registers = 0;

    switch(mode)
    {
	case 3:
	  stall_on_registers = 1;
	case 2:
	  stall_on_memory = 1;
	case 1:
	  stall_on_branches = 1;
	case 0:
	  break;
	default:
	  fprintf(stderr,"Illegal safety mode specified.\n");
	  exit(1);
      }
}


/* Simulate the code and output results to standard out */
void simulate(Instruction* code)
{
    Change* list_of_effects = NULL; 
    Change* last_effect = NULL;
    Change* new_effects;
    int instruction_count = 0;
    int operation_count = 0;
    int vect_operation_count = 0; /* Number of operations that can be vectorized. Uli  04/13/2011 */
    int cycle_count = 0;

    while(code)
    {
	if (!((memory_stall(code,list_of_effects) && stall_on_memory) ||
	      (register_stall(code,list_of_effects) && stall_on_registers) ||
	      (branch_stall(list_of_effects) && stall_on_branches)))
	{
	    if (!check_machine_constraints(code))
	    {
		fprintf(stderr,"Error: Machine constraints violated.\n");
		exit(1);
	    }
		
	    new_effects = execute_instruction(code,&operation_count,&vect_operation_count);
	    instruction_count++;

	    if (!list_of_effects)
	    {
		list_of_effects = new_effects;
		last_effect = new_effects;
	    }
	    else
		last_effect->next = new_effects;

	    /* Move last effect to end */
	    if (last_effect)
		while(last_effect->next)
		    last_effect = last_effect->next;

	    /* Go to next instruction */
	    code = code->next;
	}
	
	list_of_effects = execute_changes(list_of_effects,&last_effect,&code);
	cycle_count++;

    }

    while(list_of_effects)
    {
	list_of_effects = execute_changes(list_of_effects,&last_effect,&code);
	cycle_count++;
    }

    fprintf(stdout,"Executed %d instructions and %d operations in %d cycles.\n",
	    instruction_count,operation_count,cycle_count);
    /* fprintf(stdout,"Executed %d vectorizable operations (%d percent of overall executed operations).\n",
       vect_operation_count, (int) floor(((double) vect_operation_count / (double) operation_count) * 100)); */

}

/* Returns 1 if the instruction uses a register that is not ready */
int register_stall(Instruction* inst,Change* changes)
{
    Operation* current_op = inst->operations;

    while(current_op)
    {
	/* Check source registers for operation */
	if (!list_of_operands_ready(current_op->srcs,changes))
	    return 1;

	/* Also check target registers on stores */
	if ((opcode_specs[current_op->opcode].target_is_source) &&
	    (!list_of_operands_ready(current_op->defs,changes)))
	    return 1;

	current_op = current_op->next;
    }

    /* All registers are ready if this point is reached */
    return 0;
}

/* Returns 1 if all operands in the list are ready, and 
   return 0 if they are not */
int list_of_operands_ready(Operand* reg, Change* changes)
{
    while(reg)
    {
	/* Non zero values in register_ready indicate that
	   the register is not ready */
	if (!reg_ready(reg->value,changes))
	    return 0;
	reg = reg->next;
    }
    
    /* All register are ready if this point is reached */
    return 1;
}

/* Returns 1 if the instruction uses a memory address that is not ready */
int memory_stall(Instruction* inst, Change* changes)
{
    int memory_location;
    Operation* current_op = inst->operations;

    while(current_op)
    {
	switch(current_op->opcode)
	{
	    case LOAD:
	    memory_location = get_register(current_op->srcs->value);
	    if (!word_ready(memory_location,changes))
		return 1;
	    break;

	    case LOADAI:
	    memory_location = get_register(current_op->srcs->value) +
		current_op->consts->value;
	    if (!word_ready(memory_location,changes))
		return 1;
	    break;
	    
	    case LOADAO:
	    memory_location = get_register(current_op->srcs->value) +
		get_register(current_op->srcs->next->value);
	    if (!word_ready(memory_location,changes))
		return 1;
	    break;
	    
	    case CLOAD:
	    memory_location = get_register(current_op->srcs->value);
	    if (!mem_ready(memory_location,changes))
		return 1;
	    break;
	    
	    case CLOADAI:
	    memory_location = get_register(current_op->srcs->value) +
		current_op->consts->value;
	    if (!mem_ready(memory_location,changes))
		return 1;
	    break;

	    case CLOADAO:
	    memory_location = get_register(current_op->srcs->value) +
		get_register(current_op->srcs->next->value);
	    if (!mem_ready(memory_location,changes))
		return 1;
	    break;

	    case OUTPUT:
	    memory_location = current_op->consts->value;
	    if (!mem_ready(memory_location,changes))
		return 1;
	    break;

	    case OUTPUTAI:
	    memory_location = get_register(current_op->srcs->value) +
		current_op->consts->value;
	    if (!mem_ready(memory_location,changes))
		return 1;
	    break;

	    default:
	    break;
	
	}

	current_op = current_op->next;
    }
    /* All memory locations are ready if this point is reached */
    return 0;
}

/* Returns 1 if the register does not depend on the list of effects */
int reg_ready(int reg, Change* effects)
{
    while(effects)
    {
	if (effects->type == REGISTER && effects->location == reg)
	    return 0;
	effects = effects->next;
    }
    return 1;
}

/* Returns 1 if the memory location does not depend on the list of effects */
int mem_ready(int location, Change* effects)
{
    while(effects)
    {
	if (effects->type == MEMORY && effects->location == location)
	    return 0;
	effects = effects->next;
    }
    return 1;
}

/* Returns 1 if the word of memory does not depend on the list of effects */
int word_ready(int location, Change* effects)
{
    while(effects)
    {
	if (effects->type == MEMORY && effects->location >= location
	    && effects->location <= (location+3))
	    return 0;
	effects = effects->next;
    }
    return 1;
}

/* Execute all operations in a single instruction */
Change* execute_instruction(Instruction* inst, int* op_count, int* vect_count)
{
    Operation* current_op = inst->operations;
    Change* all_changes = NULL;
    Change* last_change;
    Change* new_changes;

    while(current_op)
    {
        if (vect_on) {     /* Uli  04/13/2011 */
	  (*vect_count)++;
        }
      
	(*op_count)++;
	new_changes = execute_operation(current_op);
	
	if (!all_changes)
	{
	    all_changes = new_changes;
	    last_change = new_changes;
	}
	else
	    last_change->next = new_changes;

	/* Move last change to end */
	if (last_change)
	    while(last_change->next)
		last_change = last_change->next;
	
		
	current_op = current_op->next;
    }

    return(all_changes);


}

/* Execute a single operation */
Change* execute_operation(Operation* op)
{
    Change* effects;
    Change* effect_ptr;
    int i;

    /* This is a big, ugly switch statement that deals with every operation */
    switch(op->opcode)
    {
	case NOP:
	  return NULL;
	  break;

	case VECTON:
          vect_on = 1;
	  return NULL;
	  break;

	case VECTOFF:
          vect_on = 0;
	  return NULL;
	  break;

	case ADD:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) + 
	      get_register(op->srcs->next->value);
	  return effects;
	  break;

	case SUB:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) - 
	      get_register(op->srcs->next->value);
	  return effects;
	  break;

	case MULT:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) * 
	      get_register(op->srcs->next->value);
	  return effects;
	  break;

	case DIV:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) / 
	      get_register(op->srcs->next->value);
	  return effects;
	  break;	  

	case ADDI:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) + op->consts->value;
	  return effects;
	  break;

	case SUBI:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) - op->consts->value;
	  return effects;
	  break;
	  
	case MULTI:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) * op->consts->value;
	  return effects;
	  break;

	case DIVI:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) / op->consts->value;
	  return effects;
	  break;

	case LSHIFT:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) << 
	      get_register(op->srcs->next->value);
	  return effects;
	  break;

	case LSHIFTI:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) << op->consts->value;
	  return effects;
	  break;

	case RSHIFT:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) >> 
	      get_register(op->srcs->next->value);
	  return effects;
	  break;

	case RSHIFTI:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) >> op->consts->value;
	  return effects;
	  break;

	case AND:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) & get_register(op->srcs->next->value);
	  return effects;
	  break;

	case OR:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) | get_register(op->srcs->next->value);
	  return effects;
	  break;

	case XOR:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value) ^ get_register(op->srcs->next->value);
	  return effects;
	  break;

	case LOADI:
	  effects = onereg(op);
	  effects->value = op->consts->value;
	  return effects;
	  break;

	case LOAD:
	  effects = onereg(op);
	  effects->value = get_word(get_register(op->srcs->value));
	  return effects;
	  break;

	case LOADAI:
	  effects = onereg(op);
	  effects->value = get_word(get_register(op->srcs->value) +
				    op->consts->value);
	  return effects;
	  break;

	case LOADAO:
	  effects = onereg(op);
	  effects->value = get_word(get_register(op->srcs->value) +
				    get_register(op->srcs->next->value));
	  return effects;
	  break;

	case CLOAD:
	  effects = onereg(op);
	  effects->value = get_memory(get_register(op->srcs->value));
	  return effects;
	  break;

	case CLOADAI:
	  effects = onereg(op);
	  effects->value = get_memory(get_register(op->srcs->value) +
				      op->consts->value);
	  return effects;
	  break;

	case CLOADAO:
	  effects = onereg(op);
	  effects->value = get_memory(get_register(op->srcs->value) +
				      get_register(op->srcs->next->value));
	  return effects;
	  break;
	  
	case STORE:
	  effect_ptr = NULL;
	  for(i=0;i<4;i++)
	  {
	      effects = storeop(op);
	      effects->value = (get_register(op->srcs->value) << (8*i)) >> 24;
	      effects->location = get_register(op->defs->value) + i;
	      effects->next = effect_ptr;
	      effect_ptr = effects;
	  }
	  return effects;
	  break;

	case STOREAI:
	  effect_ptr = NULL;
	  for(i=0;i<4;i++)
	  {
	      effects = storeop(op);
	      effects->value = (get_register(op->srcs->value) << (8*i)) >> 24;
	      effects->location = get_register(op->defs->value) + 
		  op->consts->value + i;
	      effects->next = effect_ptr;
	      effect_ptr = effects;
	  }
	  return effects;
	  break;

	case STOREAO:
	  effect_ptr = NULL;
	  for(i=0;i<4;i++)
	  {
	      effects = storeop(op);
	      effects->value = (get_register(op->srcs->value) << (8*i)) >> 24;
	      effects->location = get_register(op->defs->value) + 
		  get_register(op->defs->next->value) + i;
	      effects->next = effect_ptr;
	      effect_ptr = effects;
	  }
	  return effects;
	  break;

	case CSTORE:
	  effects = storeop(op);
	  effects->value = (get_register(op->srcs->value) << 24) >> 24;
	  effects->location = get_register(op->defs->value);
	  effects->next = NULL;
	  return effects;
	  break;

	case CSTOREAI:
	  effects = storeop(op);
	  effects->value = (get_register(op->srcs->value) << 24) >> 24;
	  effects->location = get_register(op->defs->value) + op->consts->value;
	  effects->next = NULL;
	  return effects;
	  break;

	case CSTOREAO:
	  effects = storeop(op);
	  effects->value = (get_register(op->srcs->value) << 24) >> 24;
	  effects->location = get_register(op->defs->value) + 
	      get_register(op->defs->next->value);
	  effects->next = NULL;
	  return effects;
	  break;

	case BR:
	  effects = (Change*)malloc(sizeof(Change));
	  effects->type = BRANCH;
	  effects->target = (get_label(op->labels->value))->target;
	  effects->cycles_away = opcode_specs[BR].latency;
	  effects->next = NULL;
	  return effects;
	  break;

	case CBR:
	  effects = (Change*)malloc(sizeof(Change));
	  effects->type = BRANCH;
	  if (get_register(op->srcs->value))
	      effects->target = (get_label(op->labels->value))->target;
	  else
	      effects->target = (get_label(op->labels->next->value))->target;
	  effects->cycles_away = opcode_specs[BR].latency;
	  effects->next = NULL;
	  return effects;
	  break;

	case CMPLT:
	  effects = onereg(op);
	  if (get_register(op->srcs->value) < 
	      get_register(op->srcs->next->value))
	      effects->value = 1;
	  else
	      effects->value = 0;
	  return effects;
	  break;
	  
	case CMPLE:
	  effects = onereg(op);
	  if (get_register(op->srcs->value) <= 
	      get_register(op->srcs->next->value))
	      effects->value = 1;
	  else
	      effects->value = 0;
	  return effects;
	  break;

	case CMPEQ:
	  effects = onereg(op);
	  if (get_register(op->srcs->value) == 
	      get_register(op->srcs->next->value))
	      effects->value = 1;
	  else
	      effects->value = 0;
	  return effects;
	  break;

	case CMPNE:
	  effects = onereg(op);
	  if (get_register(op->srcs->value) != 
	      get_register(op->srcs->next->value))
	      effects->value = 1;
	  else
	      effects->value = 0;
	  return effects;
	  break;

	case CMPGE:
	  effects = onereg(op);
	  if (get_register(op->srcs->value) >= 
	      get_register(op->srcs->next->value))
	      effects->value = 1;
	  else
	      effects->value = 0;
	  return effects;
	  break;

	case CMPGT:
	  effects = onereg(op);
	  if (get_register(op->srcs->value) > 
	      get_register(op->srcs->next->value))
	      effects->value = 1;
	  else
	      effects->value = 0;
	  return effects;
	  break;

	case I2I:
	  effects = onereg(op);
	  effects->value = get_register(op->srcs->value);
	  return effects;
	  break;

	case C2C:
	case C2I:
	case I2C:
	  effects = onereg(op);
	  effects->value = (get_register(op->srcs->value) << 24) >> 24;
	  return effects;
	  break;
	  
	case OUTPUT:
	  effects = (Change*)malloc(sizeof(Change));
	  effects->type = DISPLAY;
	  effects->cycles_away = opcode_specs[OUTPUT].latency;
	  effects->next = NULL;
	  effects->value = get_word(op->consts->value);
	  return effects;
	  break;
	  
	case COUTPUT:
	  effects = (Change*)malloc(sizeof(Change));
	  effects->type = DISPLAY;
	  effects->cycles_away = opcode_specs[OUTPUT].latency;
	  effects->next = NULL;
	  effects->value = get_memory(op->consts->value);
	  return effects;
	  break;

	case OUTPUTAI:
	  effects = (Change*)malloc(sizeof(Change));
	  effects->type = DISPLAY;
	  effects->cycles_away = opcode_specs[OUTPUT].latency;
	  effects->next = NULL;
	  effects->value = get_word(get_register(op->srcs->value) +
				    op->consts->value);
	  return effects;
	  break;

	default:
	  fprintf(stderr,"Simulator Error: Invalid opcode encountered in execute_operation.");
	  return NULL;
	  break;
      }
}

/* onereg creates most of a change structure for the common case where 
   a single register is defined. */
Change* onereg(Operation* op)
{
    Change* effect = (Change*)malloc(sizeof(Change));
    effect->type = REGISTER;
    effect->location = op->defs->value;
    effect->cycles_away = opcode_specs[op->opcode].latency;
    effect->next = NULL;
    return effect;
}

/* storeop creates most of a change structure for a store operation */
Change* storeop(Operation* op)
{
    Change* effect = (Change*)malloc(sizeof(Change));
    effect->type = MEMORY;
    effect->cycles_away = opcode_specs[op->opcode].latency;
    effect->next = NULL;
    return effect;
}




/* Returns 1 if there is an outstanding branch instruction */
int branch_stall(Change* changes)
{
    while(changes)
    {
	if (changes->type == BRANCH)
	    return 1;
	changes = changes->next;
    }
    return 0;
}
/*
   04/13/2011
   Uli: This seems to simulate parallel execution of operations that are able to 
        "fire" in a single cycle.
*/

/* Reduces the cycles_away of all changes by one and executes any changes
   that have a cycles_away of 0 */
Change* execute_changes(Change* changes, Change** last_change_ptr, Instruction** code_ptr)
{
    Change* first_change = changes;
    Change* prev_change = NULL;

    /* Iterate through all changes */
    while(changes)
    {
	changes->cycles_away -= 1;
	
	if (changes->cycles_away == 0)
	{
	    /* Perform effect */
	    switch(changes->type)
	    {
		case REGISTER:
		  set_register(changes->location,changes->value);
		  break;
		case MEMORY:
		  set_memory(changes->location,changes->value);
		  break;
		case BRANCH:
		  *code_ptr = changes->target;
		  break;
		case DISPLAY:
		  fprintf(stdout,"%d\n",changes->value);
		  break;
	      }

	    /* Delete change record */
	    if (prev_change)
	    {
		prev_change->next = changes->next;
		free(changes);
		changes = prev_change->next;
	    }
	    else
	    {
		first_change = changes->next;
		free(changes);
		changes = first_change;
	    }
	}
	else
	{
	    prev_change = changes;
	    changes = changes->next;
	}

    }

    *last_change_ptr = prev_change;
    return (first_change);
}

/*
  04/13/2011 
  Uli: This seems to simulate a VLIW instruction consisting of multiple
        operations. However, since there is only a limited number of functional
        units for each type, operations that fit into a single instruction are 
        limited. The limits are set here.  ???
*/
int check_machine_constraints(Instruction* inst)
{
    Operation* current_op = inst->operations;

    int memory_op_count = 0;
    int mult_op_count = 0;
    int op_count = 0;
    int output_op_count = 0;

    while(current_op)
    {
	op_count++;
	switch(current_op->opcode)
	{
	    case LOAD:   case LOADAI:   case LOADAO:
	    case CLOAD:  case CLOADAI:  case CLOADAO:
	    case STORE:  case STOREAI:  case STOREAO:
	    case CSTORE: case CSTOREAI: case CSTOREAO:
	    memory_op_count++;
	    break;

	    case MULT:
	    mult_op_count++;
	    break;

 	    case OUTPUT:
	    output_op_count++;
	    break;

	    default:
	    break;
	}

	current_op = current_op->next;
    }

    return (op_count <= 2 && mult_op_count <= 1 && memory_op_count <= 1);
}


