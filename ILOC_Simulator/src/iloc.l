WHITESPACE [ \t]
NUM [0-9]+

%{
  /* iloc.l
   * Lex specification for the ILOC subset defined in
   * "Engineering a Compiler" by Cooper and Torczon
   * written by Todd Waterman
   * 11/30/00 */

/*

2004/10/01, Jerry Hom

Modified actions when recognizing NUMs.  Storage memory for numeric
values is now allocated in the scanner rather than the parser.
Conversion of string to integer is also performed in the scanner.
Referencing 'yytext' in the parser seems to return garbage.
Complementary modifications made in the parser.


2004/10/11, Jerry Hom

Related to above, storage for labels now allocated in scanner.
Complementary modifications made in the parser.

*/

  #include <stdlib.h>
  #include <stdio.h>
  #include <string.h>

  #include "instruction.h"
  #include "y.tab.h"

  int line_counter = 1;
  Opcode* current_opcode;

%}

%%

\[            {return OPEN_BRACKET;}
\]            {return CLOSE_BRACKET;}
\;            {return SEMICOLON;}
\,            {return COMMA;}
\=\>          {return ARROW;}

%{
/* 
r{NUM}        {return REGISTER;}
{NUM}         {return NUMBER;}
\-{NUM}	      {return NUMBER;}

[a-zA-Z0-9\_]+: {return TARGET;}

[a-zA-Z0-9\_]+  { current_opcode = get_opcode(yytext);
                if (current_opcode)
                    return OPCODE;
                else
                    return LABEL;
              }
 */
%}

r{NUM}        {
  yylval.operand_ptr = (Operand *) malloc (sizeof (Operand));
  yylval.operand_ptr->value = (int) strtol (yytext + 1, (char **) NULL, 10);
  return REGISTER;
}
{NUM}         {
  yylval.operand_ptr = (Operand *) malloc (sizeof (Operand));
  yylval.operand_ptr->value = (int) strtol (yytext, (char **) NULL, 10);
  return NUMBER;
}
\-{NUM}	      {
  yylval.operand_ptr = (Operand *) malloc (sizeof (Operand));
  yylval.operand_ptr->value = -((int) strtol (yytext + 1, (char **) NULL, 10));
  return NUMBER;
}

[a-zA-Z0-9\_]+: {
  yylval.label_ptr = (char *) malloc (yyleng);
  strcpy (yylval.label_ptr, yytext);
  return TARGET;
}

[a-zA-Z0-9\_]+  { current_opcode = get_opcode(yytext);
                if (current_opcode)
                    return OPCODE;
                else {
		  yylval.label_ptr = (char *) malloc (yyleng);
		  strcpy (yylval.label_ptr, yytext);
		  return LABEL;
		}
              }

\/\/[^\n]*  { /* Comment */}

[\n]          {line_counter++;}

{WHITESPACE}  {;}

%%

yywrap()
{
  return 1;
}

