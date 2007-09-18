/*
----
This file is part of SECONDO.

Copyright (C) 2004, University in Hagen, Department of Computer Science,
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

SECONDO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----

1 Operator Syntax

[File ~opsyntax.pl~]

*/

:-
  op(800, xfx, =>),
  op(800, xfx, <=),
  op(800, xfx, #),
  op(800, xfx, div),
  op(800, xfx, mod),
  op(800, xfx, and),
  op(800, xfx, starts),
  op(800, xfx, contains),
  op(200, xfx, :).

:- op(800, xfx, inside).
:- op(800, xfx, insideold).
:- op(800, xfx, intersects).
:- op(800, xfx, adjacent).
:- op(800, xfx, attached).
:- op(800, xfx, overlaps).
:- op(800, xfx, onborder).
:- op(800, xfx, ininterior).
:- op(800, xfx, touchpoints).
:- op(800, xfx, intersection).
:- op(800, xfx, commonborder).
:- op(800, xfx, commonborderscan).

:- op(800, xfx, or).
:- op(800,  fx, not).

:- op(800, xfx, present).
:- op(800, xfx, passes).
:- op(800, xfx, atinstant).
:- op(800, xfx, atperiods).
:- op(800, xfx, at).

:- op(800, xfx, satisfies).
:- op(800, xfx, when).

:- op(800, xfx, simpleequals).

:- op(800, xfx, intersects_new).
:- op(800, xfx, p_intersects).


/*

----	secondoOp(?Op, ?Syntax, ?NoArgs) :-
----

~Op~ is a Secondo operator written in ~Syntax~, with ~NoArgs~ arguments.
Currently implemented:

  * postfix, 1 or 2 arguments: corresponds to \_ \# and \_ \_ \#

  * postfixbrackets, 2 or 3 arguments, of which the last one is put into
the brackets: \_ \# [ \_ ] or \_ \_ \# [ \_ ]

  * postfixbrackets2: \_ \# [\_ , \_] 

  * prefix, 2 arguments: \# (\_, \_)

  * prefix, either 1 or 3 arguments, does not need a rule here, is
translated by default.

  * infix, 2 arguments: does not need a rule, translated by default.

For all other forms, a plan\_to\_atom rule has to be programmed explicitly.

*/

secondoOp(distance, prefix, 2).
secondoOp(intersection_new, prefix, 2).
secondoOp(intersection, prefix, 2).
secondoOp(theperiod, prefix, 2).
secondoOp(everNearerThan, prefix, 3).
secondoOp(union_new, prefix, 2).
secondoOp(minus_new, prefix, 2).
secondoOp(feed, postfix, 1).
secondoOp(consume, postfix, 1).
secondoOp(count, postfix, 1).
secondoOp(pdelete, postfix, 1).
secondoOp(product, postfix, 2).
secondoOp(filter, postfixbrackets, 2).
secondoOp(puse, postfixbrackets, 2).
secondoOp(pfeed, postfixbrackets, 2).
secondoOp(pcreate, postfixbrackets, 2).
secondoOp(loopjoin, postfixbrackets, 2).
secondoOp(exactmatch, postfixbrackets, 3).
secondoOp(leftrange, postfixbrackets, 3).
secondoOp(rightrange, postfixbrackets, 3).
secondoOp(remove, postfixbrackets, 2).
secondoOp(sortby, postfixbrackets, 2).
secondoOp(loopsel, postfixbrackets, 2).
secondoOp(sum, postfixbrackets, 2).
secondoOp(min, postfixbrackets, 2).
secondoOp(max, postfixbrackets, 2).
secondoOp(memshuffle, postfix, 1).
secondoOp(avg, postfixbrackets, 2).
secondoOp(extract, postfixbrackets, 2).
secondoOp(tuplesize, postfix, 1).
secondoOp(exttuplesize, postfix, 1).
secondoOp(attrsize, postfixbrackets, 2).
secondoOp(head, postfixbrackets, 2).
secondoOp(windowintersects, postfixbrackets, 3).
secondoOp(windowintersectsS, postfixbrackets, 2).
secondoOp(gettuples, postfix, 2).
secondoOp(sort, postfix, 1).
secondoOp(rdup, postfix, 1).
secondoOp(bbox, prefix, 1).
secondoOp(box3d, prefix, 2).
secondoOp(symmproduct, postfix, 2).
secondoOp(move, prefix, 2).


