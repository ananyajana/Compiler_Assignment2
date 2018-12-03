%{
#include <stdio.h>
#include <string.h>
#include "attr.h"
#include "instrutil.h"
int yylex();
void yyerror(char * s);
#include "symtab.h"

FILE *outfile;
char *CommentBuffer;
 
%}

%union {tokentype token;
        regInfo targetReg;
        labelsInfo targetLabels;
        labelInfo targetLabel;
       labelsStrInfo targetLabels1;
}
%token PROG PERIOD VAR 
%token INT BOOL PRINT THEN IF DO  
%token ARRAY OF 
%token BEG END ASG  
%token EQ NEQ LT LEQ 
%token ELSE
%token FOR 
%token <token> ID ICONST 

%type <targetReg> exp 
%type <targetReg> lhs 
%type <targetReg> condexp
%type <targetLabels> ifhead
%type <targetLabels1> ctrlexp 
 

%start program

%nonassoc EQ NEQ LT LEQ 
%left '+' '-' 
%left '*' 

%nonassoc THEN
%nonassoc ELSE

%%
program : {emitComment("Assign STATIC_AREA_ADDRESS to register \"r0\"");
           emit(NOLABEL, LOADI, STATIC_AREA_ADDRESS, 0, EMPTY);} 
           PROG ID ';' block PERIOD { }
	;

block	: variables cmpdstmt { }
	;

variables: /* empty */
	| VAR vardcls { }
	;

vardcls	: vardcls vardcl ';' { }
	| vardcl ';' { }
	| error ';' { yyerror("***Error: illegal variable declaration\n");}  
	;

vardcl	: idlist ':' INT{  }
	| idlist1 ':' ARRAY '[' ICONST ']' OF INT {  }
	;

idlist	: idlist ',' ID { insert($3.str, NextOffset(1)); } /* BOGUS  - needs to be fixed */
        | ID		{ insert($1.str, NextOffset(1)); } /* BOGUS  - needs to be fixed */
	;

idlist1	: idlist1 ',' ID { insert($3.str, NextOffset(1)); } /* BOGUS  - needs to be fixed */
        | ID		{ insert($1.str, NextOffset(1)); } /* BOGUS  - needs to be fixed */
	;
type	: ARRAY '[' ICONST ']' OF INT {  }

        | INT {  }
	;

stmtlist : stmtlist ';' stmt { }
	| stmt { }
        | error { yyerror("***Error: ';' expected or illegal statement \n");}
	;

stmt    : ifstmt { }
	| fstmt { }
	| astmt { }
	| writestmt { }
	| cmpdstmt { }
	;

cmpdstmt: BEG stmtlist END { }
	;

ifstmt :  ifhead 
          THEN stmt {
			emit(NOLABEL, BR, $1.label3, EMPTY, EMPTY);
			emit($1.label2, NOP, EMPTY, EMPTY, EMPTY);}
  	  ELSE 
          stmt	{	
			int label4 = $1.label3;
			emit(label4, NOP, EMPTY, EMPTY, EMPTY);}
			
	;

ifhead : IF condexp {  	int label1 = NextLabel();
			int label2 = NextLabel();
			int label3 = NextLabel();
			emit(NOLABEL, CBR, $2.targetRegister, label1, label2);
			emit(label1, NOP, EMPTY, EMPTY, EMPTY);
			$$.label1 = label1;
			$$.label2 = label2;
			$$.label3 = label3;}
        ;

fstmt	: FOR ctrlexp { } DO stmt  { int offset = (lookup($2.str))->offset;
			
			 	  int newReg1 = NextRegister();
			 	  int newReg2 = NextRegister();
				  emit(NOLABEL, LOADAI, 0, offset, newReg1);
				  emit(NOLABEL, ADDI, newReg1, 1, newReg2);
                                 emit(NOLABEL, STOREAI, newReg2, 0, offset);
			emit(NOLABEL, BR, $2.label1, EMPTY, EMPTY);
			emit($2.label3, NOP, EMPTY, EMPTY, EMPTY);
			free($2.str);}
	;


astmt : lhs ASG exp             { 
				  emit(NOLABEL,
                                       STORE, 
                                       $3.targetRegister,
                                       $1.targetRegister,
                                       EMPTY);
                                }
	;

lhs	: ID			{ /* BOGUS  - needs to be fixed */
				  int offset;
				  if(NULL != lookup($1.str))
					offset = (lookup($1.str))->offset;
				  else
					yyerror("variable not declared\n");
                                  int newReg1 = NextRegister();
                                  int newReg2 = NextRegister();
                                  //offset = NextOffset(1);
				  
                                  $$.targetRegister = newReg2;

				  emit(NOLABEL, LOADI, offset, newReg1, EMPTY);
				  emit(NOLABEL, ADD, 0, newReg1, newReg2);
				  
                         	  }


                                |  ID '[' exp ']' {   }
                                ;

writestmt: PRINT '(' exp ')' { int printOffset = -4; /* default location for printing */
  	                         sprintf(CommentBuffer, "Code for \"PRINT\" from offset %d", printOffset);
	                         emitComment(CommentBuffer);
                                 emit(NOLABEL, STOREAI, $3.targetRegister, 0, printOffset);
                                 emit(NOLABEL, 
                                      OUTPUTAI, 
                                      0,
                                      printOffset, 
                                      EMPTY);
                               }
	;



exp	: exp '+' exp		{ int newReg = NextRegister();

                                  $$.targetRegister = newReg;
                                  emit(NOLABEL, 
                                       ADD, 
                                       $1.targetRegister, 
                                       $3.targetRegister, 
                                       newReg);
                                }

        | exp '-' exp		{  int newReg = NextRegister();

                                  $$.targetRegister = newReg;
                                  emit(NOLABEL, 
                                       SUB, 
                                       $1.targetRegister, 
                                       $3.targetRegister, 
                                       newReg);
				}

        | exp '*' exp		{  int newReg = NextRegister();

                                  $$.targetRegister = newReg;
                                  emit(NOLABEL, 
                                       MULT, 
                                       $1.targetRegister, 
                                       $3.targetRegister, 
                                       newReg);
				}


        | ID			{ /* BOGUS  - needs to be fixed */
	                          int newReg = NextRegister();
				  int offset;
				  if(NULL != lookup($1.str))
					offset = (lookup($1.str))->offset;
				  else
					yyerror("variable not declared\n");

	                          $$.targetRegister = newReg;
				  emit(NOLABEL, LOADAI, 0, offset, newReg);
                                  
	                        }

        | ID '[' exp ']'	{   }
 


	| ICONST                 { int newReg = NextRegister();
	                           $$.targetRegister = newReg;
				   emit(NOLABEL, LOADI, $1.num, newReg, EMPTY); }

	| error { yyerror("***Error: illegal expression\n");}  
	;

ctrlexp	: ID ASG ICONST ',' ICONST {
				  int offset;
				  if(NULL != lookup($1.str))
					offset = (lookup($1.str))->offset;
				  else
					yyerror("variable not declared\n");
                                  int newReg1 = NextRegister();
                                  int newReg2 = NextRegister();
                                  //offset = NextOffset(1);
				  
                                  //$$.targetRegister = newReg2;

				  emit(NOLABEL, LOADI, offset, newReg1, EMPTY);
				  emit(NOLABEL, ADD, 0, newReg1, newReg2);
				  
	                          int newReg3 = NextRegister();
	                          int newReg4 = NextRegister();

	                          int newReg5 = NextRegister();
	                          int newReg6 = NextRegister();

	                          //$$.targetRegister = newReg;
				  emit(NOLABEL, LOADI, $3.num, newReg5, EMPTY);
				  emit(NOLABEL, LOADI, $5.num, newReg6, EMPTY);
				  emit(NOLABEL,
                                       STORE, 
				       newReg5,
                                       newReg2,
                                       EMPTY);
				  //emit(NOLABEL, LOADAI, 0, offset, newReg2);
				  $$.label1 = NextLabel();
				  $$.label2 = NextLabel();
				  $$.label3 = NextLabel();
			 	  
				  emit($$.label1, LOADAI, 0, offset, newReg3);
				   emit(NOLABEL, CMPLE, newReg3, newReg6, newReg4); 
			  emit(NOLABEL, CBR, newReg4, $$.label2, $$.label3);
				//printf("%d %c %s\n", ID, ID, ID);
			emit($$.label2, NOP, EMPTY, EMPTY, EMPTY);
			$$.str = (char*)malloc(strlen($1.str) + 1);
			strcpy($$.str, $1.str);

				  			
	
	}
	| error { yyerror("***Error: illegal control expression\n");}  
        ;


condexp	: exp NEQ exp		{  int newReg = NextRegister();
				   emit(NOLABEL, CMPNE, $1.targetRegister, $3.targetRegister, newReg);
                                   $$.targetRegister= newReg;}

        | exp EQ exp		{  int newReg = NextRegister();
				   emit(NOLABEL, CMPEQ, $1.targetRegister, $3.targetRegister, newReg); 
                                   $$.targetRegister= newReg;}

        | exp LT exp		{  int newReg = NextRegister();
				   emit(NOLABEL, CMPLT, $1.targetRegister, $3.targetRegister, newReg); 
                                   $$.targetRegister= newReg;}

        | exp LEQ exp		{  int newReg = NextRegister();
				   emit(NOLABEL, CMPLE, $1.targetRegister, $3.targetRegister, newReg); 
                                   $$.targetRegister= newReg;}

	| error { yyerror("***Error: illegal conditional expression\n");}  
        ;

%%

void yyerror(char* s) {
        fprintf(stderr,"%s\n",s);
        }


int
main(int argc, char* argv[]) {

  printf("\n     CS515 Fall 2018 Compiler\n\n");

  outfile = fopen("iloc.out", "w");
  if (outfile == NULL) { 
    printf("ERROR: cannot open output file \"iloc.out\".\n");
    return -1;
  }

  CommentBuffer = (char *) malloc(650);  
  InitSymbolTable();

  printf("1\t");
  yyparse();
  printf("\n");

  PrintSymbolTable();
  
  fclose(outfile);
  
  return 1;
}




