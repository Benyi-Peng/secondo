/*
----
This file is part of the PD system

Copyright (C) 1998 Ralf Hartmut Gueting, 
          (C) 2006 Markus Spiekermann

Fachbereich Informatik, FernUniversitaet in Hagen

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

Use the parser to transform from implicitly formatted text to TeX. The auxiliary
functions CheckDebugEnv, InitHeader and PrintTail are implemented in file PDLib.c.

*/

#include <stdio.h>
#include <stdlib.h>
#include "PDLib.h"

extern int yyparse();

int main()
{

  CheckDebugEnv();
  InitHeader();
   
  int error=0;
  error = yyparse();

  PrintTail();
  return error;
}


