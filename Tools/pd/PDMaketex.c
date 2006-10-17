/*
----
This file is part of the PD system
Copyright (C) 1998 Ralf Hartmut Gueting, Fachbereich Informatik, FernUniversitaet Hagen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
----

6.2 Main Program: Maketex.c

Use the parser to transform from implicitly formatted text to TeX.

*/

#include <stdio.h>
#include <stdlib.h>

char*  startProgram = "{\\small \\begin{quote} \\begin{verbatim}\n";
char*  endProgram = "\n\\end{verbatim} \\end{quote}}\n\n";

char*  startVerbatim = "\\begin{pdverbatim}\n    ";
char*  endVerbatim = "\n\\end{pdverbatim}\n"; 

// debugging flags for flex and bison
extern int yydebug;
extern int yy_flex_debug;

int main()
{

  yydebug=0;
  yy_flex_debug=0;
  char* d = getenv("PD_DEBUG");
  if (d) {
    yydebug = atoi(d);
    yy_flex_debug = yydebug;
  }  

  char* s = getenv("PD_HEADER");
  if (s) {
    const char* lst="listing";
    // check if value of s ends with "listing"
    char* cs = strstr(s,lst);
    if ( cs && strcmp(cs,lst) == 0 ) 
    {
      startProgram = "\n\\begin{pdprogram}\\begin{lstlisting}\n";
      endProgram = "\\end{lstlisting}\\end{pdprogram}\n";
    } 
  }
   
  int error=0;
  error = yyparse();
  print_tail();
  return error;
}

print_tail()
{
	printf("\\end{document}\n");
}	

