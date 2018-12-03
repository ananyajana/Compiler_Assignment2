/* iloc.y
 * Yacc specification for the ILOC subset defined in
 * "Engineering a Compiler" by Cooper and Torczon
 * written by Todd Waterman
 * 11/30/00 */

/*
  2004/10/01, Jerry Hom

  Modified actions for rules recognizing numeric values, which include
  REG and CONST.  Storage memory for numeric values is now allocated
  in the scanner rather than the parser.  Conversion of string to
  integer is also performed in the scanner.  Referencing 'yytext' in
  the parser seems to return garbage.  Complementary modifications
  made in the scanner.

  I also commented out some free() statements which seem to be
  incorrect, though they're only triggered when a rule contains an
  'operand_list' item.  Might never occur, or only on complex
  programs, but it just seems like an obvious bug which should be
  corrected.


  2004/10/11, Jerry Hom

  Related to above, storage for labels now allocated in scanner.  I
  added 'label_ptr' into the token data structure.  The tokens LABEL
  and TARGET also have declared 'label_ptr' types.
*/

%{
  #include <stdio.h>
  #include <string.h>
  #include "instruction.h"

  #define MAX_ERROR_MESSAGE_LENGTH 100

  Operands* new_operands(void);
  Operand* append_operands(Operand*,Operand*);
  int verify_args(Opcode*,int,int,int,int);

  extern char yytext[];

  extern int line_counter;
  extern Opcode* current_opcode;

  /* This function must be defined */
  void yyerror(char*);

  /* If an error is encountered during parsing this is changed to 1 */
  int error_found = 0;

  /* Pointer to the first instruction */
  Instruction* first_instruction;

%}

/* 
%union {
    int ival;
    Instruction* inst_ptr;
    Operation* op_ptr;
    Operands* operands_ptr;
    Operand* operand_ptr;
    Opcode* opcode_ptr;
}
*/

%union {
    int ival;
    Instruction* inst_ptr;
    Operation* op_ptr;
    Operands* operands_ptr;
    Operand* operand_ptr;
    Opcode* opcode_ptr;
  char* label_ptr;
}

%token OPEN_BRACKET
%token CLOSE_BRACKET
%token SEMICOLON
%token COMMA
%token ARROW
%token OPCODE
%token OUTPUT
%token REGISTER
%token NUMBER
/* %token LABEL
   %token TARGET */
%token <label_ptr> LABEL
%token <label_ptr> TARGET

%type <inst_ptr> instruction_list
%type <inst_ptr> instruction
%type <op_ptr> operation_list
%type <op_ptr> operation
%type <opcode_ptr> the_opcode
%type <operands_ptr> operand_list
%type <operand_ptr> reg
%type <operand_ptr> const
%type <operand_ptr> lbl
%type <ival> label_def

%start iloc_program

%% /* Beginning of rules */

iloc_program     : instruction_list
                 {
		     first_instruction = $1;
		 }
                 ;

instruction_list : instruction
                 {
		     $$ = $1;
		 }
                 | label_def instruction
                 {
		     Label* label_definition = get_label($1);
		     label_definition->target = $2;
		     $$ = $2;
		 }
                 | instruction instruction_list
                 {
		     $1->next = $2;
		     $$ = $1;
		 }
                 | label_def instruction instruction_list
                 {
		     Label* label_definition = get_label($1);
		     label_definition->target = $2;
		     $2->next = $3;
		     $$ = $2;
		 }
                 ;

instruction      : operation
                 {
		     $$ = malloc(sizeof(Instruction));
		     $$->operations = $1;
		     $$->next = NULL;
		 }
                 | OPEN_BRACKET operation_list CLOSE_BRACKET
                 {
		     $$ = malloc(sizeof(Instruction));
		     $$->operations = $2;
		     $$->next = NULL;
		 }
                 ;

operation_list   : operation
                 {
		     $$ = $1;
		 }
                 | operation SEMICOLON operation_list
                 {
		     $1->next = $3;
		     $$ = $1;
		 }
                 ;

operation        : the_opcode operand_list ARROW operand_list
                 {
		     verify_args($1,$2->num_regs,$2->num_consts+$4->num_consts,
				 $2->num_labels+$4->num_labels,$4->num_regs);
		     $$ = malloc(sizeof(Operation));
		     $$->opcode = $1->name;
		     $$->srcs = $2->regs;
		     $$->consts = append_operands($2->consts,$4->consts);
		     $$->labels = append_operands($2->labels,$4->labels);
		     $$->defs = $4->regs;
		     $$->next = NULL;
		     /* 		     free($2);
					     free($4); */
		 }
                 | the_opcode operand_list
                 {
		     verify_args($1,$2->num_regs,$2->num_consts,$2->num_labels,0);
		     $$ = malloc(sizeof(Operation));
		     $$->opcode = $1->name;
		     $$->srcs = $2->regs;
		     $$->consts = $2->consts;
		     $$->labels = $2->labels;
		     $$->defs = NULL;
		     $$->next = NULL;
		     /* 		     free($2); */
		 }
                 | the_opcode ARROW operand_list
                 {
		     verify_args($1,0,$3->num_consts,$3->num_labels,$3->num_regs);
		     $$ = malloc(sizeof(Operation));
		     $$->opcode = $1->name;
		     $$->srcs = NULL;
		     $$->consts = $3->consts;
		     $$->labels = $3->labels;
		     $$->defs = $3->regs;
		     $$->next = NULL;
		     /* 		     free($3); */
		 }
                 | the_opcode
                 {
		     verify_args($1,0,0,0,0);
		     $$ = malloc(sizeof(Operation));
		     $$->opcode = $1->name;
		     $$->srcs = NULL;
		     $$->consts = NULL;
		     $$->labels = NULL;
		     $$->defs = NULL;
		     $$->next = NULL;
		 }
                 ;

the_opcode       : OPCODE
                 {
		     $$ = current_opcode;
		 }
                 ;

operand_list     : reg
                 {
		     $$ = new_operands();
		     $$->num_regs = 1;
		     $$->regs = $1;
		 }
                 | reg COMMA operand_list
                 {
		     $$ = $3;
		     $$->num_regs += 1;
		     $1->next = $$->regs;
		     $$->regs = $1;
		 }
                 | const
                 {
		     $$ = new_operands();
		     $$->num_consts = 1;
		     $$->consts = $1;
		 }
                 | const COMMA operand_list
                 {
		     $$ = $3;
		     $$->num_consts += 1;
		     $1->next = $$->consts;
		     $$->consts = $1;
		 }
                 | lbl
                 {
		     $$ = new_operands();
		     $$->num_labels = 1;
		     $$->labels = $1;
		 }
                 | lbl COMMA operand_list
                 {
		     $$ = $3;
		     $$->num_labels += 1;
		     $1->next = $$->labels;
		     $$->labels = $1;
		 }
                 ;

reg              : REGISTER
                 {
		   /* 		     $$ = malloc(sizeof(Operand));
				     $$->value = (int) strtol(&yytext[1], (char**) NULL, 10); */
		     $$->next = NULL;
		 }
                 ;

const            : NUMBER
                 {
		   /* 		     $$ = malloc(sizeof(Operand));
				     $$->value = (int) strtol(yytext, (char**) NULL, 10); */
		     $$->next = NULL;
		 }
		 ;

/* POTENTIAL malloc() PROBLEM WITH lbl AND label_def, SIMILAR TO const
   SEE instruction.c FOR insert_label() */
lbl              : LABEL
                 {
		   /* 		     $$ = malloc(sizeof(Operand));
				     $$->value = insert_label(yytext); */
		     $$->value = insert_label($1);
		     $$->next = NULL;
		 }
                 ;

label_def        : TARGET
                 {
		   /* 		     int last_char = strlen(yytext) - 1;
		     yytext[last_char] = '\0';
		     $$ = insert_label(yytext); */
		     int last_char = strlen($1) - 1;
		     $1[last_char] = '\0';
		     $$ = insert_label($1);
		 }
                 ;

%% /* Support Code */

/* Create a new initialized Operands structure */
Operands* new_operands()
{
    Operands* operands_ptr = malloc(sizeof(Operands));
    operands_ptr->num_regs = 0;
    operands_ptr->regs = NULL;
    operands_ptr->num_consts = 0;
    operands_ptr->consts = NULL;
    operands_ptr->num_labels = 0;
    operands_ptr->labels = NULL;
    
    return(operands_ptr);
}

/* Append the second list of operands to the end of the first */
Operand* append_operands(Operand* list1, Operand* list2)
{
    Operand* start = list1;

    if (!list1)
	return list2;
    
    while(list1->next)
	list1 = list1->next;

    list1->next = list2;

    for (list1 = start; list1->next; list1 = list1->next) {
      printf ("list of appended operands:  %i\t", list1->value);
    }

    return(start);
}

/* Make sure that the operation was called with the correct number and type
   of arguments */
int verify_args(Opcode* operation,int srcs, int consts, int labels, int defs)
{
    char* error_message;

    if (operation->srcs != srcs)
    {
        error_message = malloc(MAX_ERROR_MESSAGE_LENGTH*sizeof(char));
	sprintf(error_message,"%s used with incorrect number of source registers",
		operation->string);
	yyerror(error_message);
	free(error_message);
	return 0;
    }
    
    if (operation->consts != consts)
    {
        error_message = malloc(MAX_ERROR_MESSAGE_LENGTH*sizeof(char));
	sprintf(error_message,"%s used with incorrect number of constants",
		operation->string);
	yyerror(error_message);
	free(error_message);
	return 0;
    }

    if (operation->labels != labels)
    {
        error_message = malloc(MAX_ERROR_MESSAGE_LENGTH*sizeof(char));
	sprintf(error_message,"%s used with incorrect number of labels",
		operation->string);
	yyerror(error_message);
	free(error_message);
	return 0;
    }

    if (operation->defs != defs)
    {
        error_message = malloc(MAX_ERROR_MESSAGE_LENGTH*sizeof(char));
	sprintf(error_message,"%s used with incorrect number of defined registers",
		operation->string);
	yyerror(error_message);
	free(error_message);
	return 0;
    }

    return 1;
}
    
	
void yyerror(char* s)
{
  (void) fprintf(stderr, "%s at line %d\n", s, line_counter);
  error_found = 1;
}
