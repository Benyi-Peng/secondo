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

1 Auxiliary Predicates

This file contains the pretty-printing predicate
~pretty\_print~ and various auxiliary predicates for
~pretty\_print~ and a ~secondo~ predicate which uses just
one argument (the command) and pretty-prints the result.

1.1 Predicate ~pretty\_print~

Predicate ~pretty\_print~ prints a list L which is assumed to
be a PROLOG representation of a
Secondo nested list. That is the case e.g.
if L is output by the ~secondo~ predicate. If L is a relation,
a special output format is used which makes reading the
output more comfortable. That output format closely resembles
the output format used by SecondoTTY.

1.1.1 Predicates Auxiliary to Predicate ~pretty\_print~

*/
is_atomic_list([]).
is_atomic_list([Head | Tail]) :-
  atomic(Head),
  is_atomic_list(Tail).

write_spaces(0).

write_spaces(N) :-
  N > 0,
  write(' '),
  N1 is N - 1,
  write_spaces(N1).

write_tabs(N) :-
  N1 is 2 * N ,
  write_spaces(N1).

write_atoms([X]) :-
  !,
  write(X).

write_atoms([X | Rest]) :-
  write(X),
  write(', '),
  write_atoms(Rest).

write_element(X, N) :-
  atomic(X),
  write_tabs(N),
  write(X).

write_element(X, N) :-
  is_atomic_list(X),
  !,
  write_tabs(N),
  write('['),
  write_atoms(X),
  write(']').

write_element(X, N) :-
  is_list(X),
  N1 is N + 1,
  write_tabs(N),
  write('['),
  nl,
  write_elements(X, N1),
  write(']').

write_elements([], _).

write_elements([X], N) :-
  !,
  write_element(X, N).

write_elements([X | L], N) :-
  write_element(X, N),
  write(','),
  nl,
  write_elements(L, N).

max_attr_length([], 0).

max_attr_length([[Name, _] | AttrDescription], M) :-
  max_attr_length(AttrDescription, M1),
  atom_length(Name, M2),
  M is max(M1, M2).

write_tuple([], [], _).

write_tuple([[Name, _] | RestOfAttr], [AttrValue | RestOfValues], M) :-
  write(Name),
  atom_length(Name, NLength),
  PadLength is M - NLength,
  write_spaces(PadLength),
  write(' : '),
  write(AttrValue),
  nl,
  write_tuple(RestOfAttr, RestOfValues, M).

write_tuples(_, [], _).

write_tuples(AttrDescription, [Tuple], M) :-
  !,
  write_tuple(AttrDescription, Tuple, M).

write_tuples(AttrDescription, [Tuple | TupleList], M) :-
  write_tuple(AttrDescription, Tuple, M),
  nl,
  write_tuples(AttrDescription, TupleList, M).

/*

1.1.2 Predicate ~pretty\_print~

*/

pretty_print([[rel, [tuple, AttrDescription]], Tuples]) :-
  !,
  nl,
  max_attr_length(AttrDescription, AttrLength),
  write_tuples(AttrDescription, Tuples, AttrLength).

pretty_print(L) :-
  write_element(L, 0).

/*

1.1.2 Predicate ~show~

*/


show([Type, Value]) :-
  !,
  write('type : '),  write(Type),nl,
  write('value : '), write(Value),nl,
  display(Type, Value).

show(Y) :-
  pretty_print(Y),
  nl.

/*

1.1.3 Predicate ~display~

----	display(Type, Value) :-
----

Display the value according to its type description. To be extended when new
type constructors are added to Secondo.

*/



display(int, N) :-
  !, 
  write(N).

display(real, N) :-
  !, 
  write(N).

display(bool, N) :-
  !, 
  write(N).

display(string, N) :-
  !,
  term_to_atom(String, N), 
  displayString(String).
  
display(instant, N) :-
  !,
  term_to_atom(String, N), 
  displayString(String).
  
display(text, N) :-
  !,
  is_list(N),
  write_elements(N, 0).

display(rect, [L, R, B, T]) :-
  !,
  write('rectangle xl = '), write(L),
  write(', xr = '), write(R),
  write(', yb = '), write(B),
  write(', yt = '), write(T).

display([rel, [tuple, Attrs]], Tuples) :-
  !,
  nl,
  max_attr_length(Attrs, AttrLength),
  displayTuples(Attrs, Tuples, AttrLength).

display([array, Type], [], _).

display([array, Type], [First | Rest], Count) :- 
  write('---------- '),
  TCount is Count + 1,
  write(TCount),
  write(' ----------'),nl,
  display(Type, First),nl,
  display([array, Type], Rest, TCount).

display([array, Type], Value) :-
  !,  
  display([array, Type], Value, 0).

display(Type, Value) :-
  write('There is no specific display function for type '), write(Type),
  write('. '),
  nl,
  write('Generic display used. '),
  nl,
  pretty_print(Value),
  nl.


displayString([]).

displayString([Char | Rest]) :- 
  put(Char), 
  displayString(Rest).

displayTuples(_, [], _).

displayTuples(Attrs, [Tuple | Rest], AttrLength) :-
  displayTuple(Attrs, Tuple, AttrLength),
  nl,
  displayTuples(Attrs, Rest, AttrLength).


displayTuple([], _, _).

displayTuple([[Name, Type] | Attrs], [Value | Values], AttrNameLength) :-
  atom_length(Name, NLength),
  PadLength is AttrNameLength - NLength,
  write_spaces(PadLength),
  write(Name),
  write(' : '),
  display(Type, Value),
  nl,
  displayTuple(Attrs, Values, AttrNameLength).


/*

1.2 Predicate ~secondo~


Predicate ~secondo~ expects its argument to be a string atom or
a nested list, representing a query to the SECONDO system. The query is
executed and the result pretty-printed. If the query fails, the error code
and error message are printed.

*/
secondo(X) :-
  sub_atom(X,0,4,_,S),
  atom_prefix(S,'open'),	
  secondo(X, Y),
  retract(storedDatabaseOpen(_)),
  assert(storedDatabaseOpen(1)),
  getSecondoList(_),
  write('Command succeeded, result:'),
  nl, nl,
  show(Y),!.

secondo(X) :-
  sub_atom(X,0,4,_,S),
  atom_prefix(S,'open'),
  secondo_error_info(ErrorCode, ErrorString),
  write('Command failed with error code : '),
  write(ErrorCode),
  nl,
  write('and error message : '),
  nl,
  write(ErrorString),
  nl,
  !,
  fail.

secondo(X) :-
  sub_atom(X,0,5,_,S),
  atom_prefix(S,'close'),	
  secondo(X, Y),
  retract(storedDatabaseOpen(_)),
  assert(storedDatabaseOpen(0)),
  retract(storedSecondoList(_)),
  write('Command succeeded, result:'),
  nl, nl,
  show(Y),!.

secondo(X) :-
  sub_atom(X,0,5,_,S),
  atom_prefix(S,'close'),
  secondo_error_info(ErrorCode, ErrorString),
  write('Command failed with error code : '),
  write(ErrorCode),
  nl,
  write('and error message : '),
  nl,
  write(ErrorString),
  nl,
  !,
  fail.
  
secondo(X) :-
  sub_atom(X,0,6,_,S),
  atom_prefix(S,'update'),
  isDatabaseOpen,  
  secondo(X, Y),
  retract(storedSecondoList(_)),
  getSecondoList(_),
  write('Command succeeded, result:'),
  nl, nl,
  show(Y),!.

secondo(X) :-
  sub_atom(X,0,6,_,S),
  atom_prefix(S,'update'),
  isDatabaseOpen,
  secondo_error_info(ErrorCode, ErrorString),
  write('Command failed with error code : '),
  write(ErrorCode),
  nl,
  write('and error message : '),
  nl,
  write(ErrorString),
  nl,
  !,
  fail.

secondo(X) :-
  sub_atom(X,0,6,_,S),
  atom_prefix(S,'derive'),
  isDatabaseOpen,  
  secondo(X, Y),
  retract(storedSecondoList(_)),
  getSecondoList(_),
  write('Command succeeded, result:'),
  nl, nl,
  show(Y),!.

secondo(X) :-
  sub_atom(X,0,6,_,S),
  atom_prefix(S,'derive'),
  isDatabaseOpen,
  secondo_error_info(ErrorCode, ErrorString),
  write('Command failed with error code : '),
  write(ErrorCode),
  nl,
  write('and error message : '),
  nl,
  write(ErrorString),
  nl,
  !,
  fail.

secondo(X) :-
  sub_atom(X,0,3,_,S),
  atom_prefix(S,'let'),
  isDatabaseOpen, 
  secondo(X, Y),
  retract(storedSecondoList(_)),
  getSecondoList(_),
  write('Command succeeded, result:'),
  nl, nl,
  show(Y),!.

secondo(X) :-
  sub_atom(X,0,3,_,S),
  atom_prefix(S,'let'),
  isDatabaseOpen, 
  secondo_error_info(ErrorCode, ErrorString),
  write('Command failed with error code : '),
  write(ErrorCode),
  nl,
  write('and error message : '),
  nl,
  write(ErrorString),
  nl,
  !,
  fail.
  
secondo(X) :-
  sub_atom(X,0,6,_,S),
  atom_prefix(S,'create'),
  sub_atom(X,0,15,_,S),
  not(atom_prefix(S,'create database')),
  isDatabaseOpen,  
  secondo(X, Y),
  retract(storedSecondoList(_)),
  getSecondoList(_),
  write('Command succeeded, result:'),
  nl, nl,
  show(Y),!.

secondo(X) :-
  sub_atom(X,0,6,_,S),
  atom_prefix(S,'delete'),
  sub_atom(X,0,15,_,S),
  not(atom_prefix(S,'delete database')),
  isDatabaseOpen,  
  secondo(X, Y),
  retract(storedSecondoList(_)),
  getSecondoList(_),
  write('Command succeeded, result:'),
  nl, nl,
  show(Y),!.
  
secondo(X) :-
  (
    secondo(X, Y),
    write('Command succeeded, result:'),
    nl, nl,
    show(Y)
  );(
    secondo_error_info(ErrorCode, ErrorString),
    write('Command failed with error code : '),
    write(ErrorCode),
    nl,
    write('and error message : '),
    nl,
    write(ErrorString),
    nl,
    !,
    fail
  ).

/*

1.3 Operators ~query~, ~update~, ~let~, ~create~, ~open~, and ~delete~

The purpose of these operators is to make using the PROLOG interface
similar to using SecondoTTY. A SecondoTTY query

----    query ten
----

can be issued as

----    query 'ten'.
----

in the PROLOG interface via the ~query~ operator. The operators
~delete~, ~let~, ~create~, ~open~, and ~update~ work the same way.

*/
isDatabaseOpen :-
  storedDatabaseOpen(Status),
  Status = 1, !.
  
isDatabaseOpen :-
  storedDatabaseOpen(Status),
  Status = 0,
  write('No database open.'),
  nl,
  !,fail.

notIsDatabaseOpen :-
  storedDatabaseOpen(Status),
  Status = 0.

query(Query) :-
  isDatabaseOpen,
  atom(Query),
  atom_concat('query ', Query, QueryText),
  secondo(QueryText).

let(Query) :-
  isDatabaseOpen,
  atom(Query),
  atom_concat('let ', Query, QueryText),
  secondo(QueryText).
  %retract(storedSecondoList(_)),
  %getSecondoList(_).  

derive(Query) :-
  isDatabaseOpen,
  atom(Query),
  atom_concat('derive ', Query, QueryText),
  secondo(QueryText).
  %retract(storedSecondoList(_)),
  %getSecondoList(_). 

create(Query) :-
  notIsDatabaseOpen,
  atom(Query),
  atom_concat('create ', Query, QueryText),
  !,
  secondo(QueryText).

create(Query) :-
  isDatabaseOpen,
  atom(Query),
  atom_concat('create ', Query, QueryText),
  secondo(QueryText),
  retract(storedSecondoList(_)),
  getSecondoList(_).

update(Query) :-
  isDatabaseOpen,
  atom(Query),
  atom_concat('update ', Query, QueryText),
  secondo(QueryText).
  %retract(storedSecondoList(_)),
  %getSecondoList(_).
    
delete(Query) :-
  notIsDatabaseOpen,
  atom(Query),
  atom_concat('delete ', Query, QueryText),
  !,
  secondo(QueryText).

delete(Query) :-
  isDatabaseOpen,
  atom(Query),
  atom_concat('delete ', Query, QueryText),
  secondo(QueryText),
  retract(storedSecondoList(_)),
  getSecondoList(_).

open(Query) :-
  atom(Query),
  atom_concat('open ', Query, QueryText),
  secondo(QueryText),
  retract(storedDatabaseOpen(_)),
  assert(storedDatabaseOpen(1)),
  getSecondoList(_).

:-
  op(800, fx, query),
  op(800, fx, delete),
  op(800, fx, let),
  op(800, fx, create),
  op(800, fx, open),
  op(800, fx, derive),
  op(800, fx, update).
  
:- dynamic(storedDatabaseOpen/1).
storedDatabaseOpen(0).
