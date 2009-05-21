/*

----
This file is part of SECONDO.

Copyright (C) 2004-2008, University in Hagen, Faculty of Mathematics and
Computer Science, Database Systems for New Applications.

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

*/
/*
2 Part one - Determine relations and sql predicates using by the sql query

2.1 Goal of part one

  Goal of part one is to determine any relation and any sql predicate
used by sql query to be optimized. This information will be used by 
Part two to construct execution plans for secondo database to
estimate the selectivity of joined predicates.

2.2 The algorithm in few words

  1 Find any used sql predicate by using predicate pattern edge(0,A,B,\_,\_,\_).

    The zero as argument 1 means only edges should be used
    starting at start node. ???For the result B is an identifier
    of a marginal predicate ever and all edges using only one predicate
    match the pattern.??? A unifies with POG identifier
    of sql predicate unified with B.

  2 Based on unifiers of??? B the relations of sql query can be identified.
    The directly computed??? list could contain a relation twice:

    1 two instances of one relation (different aliases)

    2 one instance of one relation twice (used by different sql predicates)

    In the first case nothing is to be done. In the second case the 
    second occurence of the relation shouldn't be added to the list of 
    relations.

  3 A list is built containing the relations found. This list is stored as
    parameter of dynamic fact listOfRelations/1.

  4 Also based on unifiers of B the sql predicates of sql query are searched.

  5 The list of sql predicates generated before is stored as parameter of
    the dynamic fact listOfPredicates/1.

*/
/*
2.3 The defined dynamic facts

2.3.1 listOfPredicates/1

  The dynamic fact listOfPredicates/1 exists only once. The
argument is
a list containing the sql predicates of the sql query. The predicates
are represented as a list of tuples. These tuples consist three elements:

  * the POG identifier of the sql predicate,

  * the relation of the sql predicate as predicate rel/3 and
 
  * the sql predicate itself.

2.3.2 listOfRelations/1

  The dynamic fact listOfRelations/1 exists only once, too. The 
argument
is also a list, but it contains the instances of the relations used 
by the sql predicates of the sql query. Each relation instance is
represented by 
the predicate rel/3.The list is distinctive. Nevertheless it could contain 
one relation twice, if the relation is used with different aliases.

2.3.2.1 instances of relations

  One relation can be used in a sql query more than one time 
by defining distinctive aliases. 
An instance of a relation is identified uniquely??? by an alias or just by
the name of the relation if no alias was defined. Each sql predicate 
is exactly bundled with one relation instance.

*/
/*
2.4 The implementation of the predicates

2.4.1 findRelation/2

  The predicate is used to find a relation (arg 1) in an given list (arg 2)
of relations. If arg 1 could be found in arg 2, so the predicate finishs 
successfully otherwise it fails.

*/

% just for debugging issues
findRelation(Relation, [] ):- % Relation not found in list
	debug_writeln(findRelation/2, 70, [ Relation, ' could not be found in list' ]),
	!, % do not call unexpected call
	fail. 

% the element heading the list is be able to unify with argument one
% (it means arg1 could be found in arg2) - predicate successful
findRelation(SearchFor, [ Relation | _ ]):- % Relation found
	set_equal(SearchFor, Relation),
	debug_writeln(findRelation/2, 71, [ 'found/equals ', SearchFor, ' in/and ', Relation ]).

% the element heading the list couldn't be unified with argument 1, 
% but the list is not empty
% try again with the next top element of list
findRelation(SearchFor, [ _ | Rest ]):-
	debug_writeln(findRelation/2, 75, [ ' look for ', SearchFor, ' in the rest ', Rest ]),
	!, % do not call unexpected call
	findRelation(SearchFor, Rest).

% unexpected call
findRelation(A, B):-
	not(B=[]),
	error_exit(findRelation/2, [ 'unexpected call with arguments: ', [A, B] ]).


% VJO ??? - neu Doku
set_equal(Set, Set):-
	debug_writeln(set_equal/2, 81, [ 'lists ', Set, ' and ', Set, ' are equal']).
% this definition is just for performance issues
set_equal([Elem1,Elem2], [Elem2,Elem1]):-
	debug_writeln(set_equal/2, 81, [ 'sets ', [Elem1,Elem2], ' and ', [Elem2,Elem1], ' are equal']).
set_equal(Set1, Set2):-
	subset(Set1,Set2),
	subset(Set2,Set1),
	debug_writeln(set_equal/2, 81, [ 'sets ', Set1, ' and ', Set2, ' are equal']).
% just for debugging issues
set_equal(Set1, Set2):-
	debug_writeln(set_equal/2, 80, [ 'sets ', Set1, ' and ', Set2, ' are not equal']),
	fail.


/*
2.4.2 add\_relation/1

  The predicate appends the list of relations by the given 
relation (arg 1),
if arg 1 is not still a part of the list. The list of relations is stored 
in fact listOfRelations/1.

*/


% the given relation is still a part of the list of relations - 
% do not add the relation 
add_relation(Relation):-
	listOfRelations(ListRelations),
	%!, % if relation not in list - add_relation 1 have to fail
	findRelation(Relation , ListRelations),
	debug_writeln(add_relation/1, 62, [ Relation, ' is still a part of listOfRelations ', ListRelations, ' (do not apped)' ]),
	!. % if Relation could be found - do not search again

% append list of relations by the given relation
add_relation(Relation):-
%Q%	listOfRelations(OldList),
	retract(listOfRelations(OldList)),
	NewList = [ Relation | OldList ],
	asserta(listOfRelations(NewList)),
	debug_writeln(add_relation/1, 61, [ ' append listOfRelations ', OldList, ' by ', Relation ]),
	!.
% unexpected call
add_relation(A):-
	error_exit(add_relation/1, [ 'unexpected call with argument: ', A ]).

/*
2.4.3 add\_predicate/1

  The predicate appends the list of sql predicates always found
by sql predicate (arg 1). Finally the list of sql predicates
is stored as argument of fact listOfPredicates/1.

*/

add_predicate(Predicate):-
%Q%	listOfPredicates(OldList),
	retract(listOfPredicates(OldList)),
	NewList = [ Predicate | OldList ],
	asserta(listOfPredicates(NewList)),
	debug_writeln(add_predicate/1, 61, [ ' append listOfPredicates ', OldList, ' by ', Predicate ]),
	!.
% unexpected call
add_predicate(A):-
	error_exit(add_predicate/1, [ 'unexpected call with argument: ', A ]).


/*
2.4.4 build\_list\_of\_predicates/1

  The predicate is the entry point of part one. It initiates 
the construction of the list of relations and the list of predicates
finally stored as argument of facts listOfRelations/1 and
listOfPredicates/1.

**something to do ?**

*/
build_list_of_predicates:-
	% clean up environment
	retractall(listOfRelations(_)),
	retractall(listOfPredicates(_)),
	asserta(listOfPredicates([])),
	asserta(listOfRelations([])),
%?%	!, kann dann "unexpected call" noch erreicht werden?
	% get all Predicates
	findall([PredIdent,Pr], edge(0,PredIdent,Pr,_,_,_), PredList),
	debug_writeln(build_list_of_predicates/0, 31, [ 'all predicates added to list ', PredList]),
	% build list of relations and list of predicates
	build_list_of_predicates2(PredList).

% unexpected call
build_list_of_predicates:-
	error_exit(build_list_of_predicates/0, [ 'unexpected call' ]).

build_list_of_predicates2([]):-
	debug_writeln(build_list_of_predicates2/1, 42, [ 'all predicates evaluated']).

build_list_of_predicates2([ [ PredIdent,Pr ] | Rest ]):-
	get_predicate_contents(Pr, PredExpr, Relation),
	build_list_of_predicates3(PredIdent, PredExpr, Relation),
	debug_writeln(build_list_of_predicates2/1, 41, [ 'predicate tuple ', [PredIdent,Pr], ' evaluated' ]),
	build_list_of_predicates2(Rest).

build_list_of_predicates2([ [ PredIdent,Pr ] | Rest ]):-
	debug_writeln(build_list_of_predicates2/1, 40, [ 'predicate tuple ', [PredIdent,Pr], ' not supported - ignore it']),
	build_list_of_predicates2(Rest).

% unexpected call
build_list_of_predicates2(A):-
	error_exit(build_list_of_predicates2/1, [ 'unexpected call with argument ', A ]).

build_list_of_predicates3(PredIdent, PredExpr, Relation):-
	% memory structure to store a found predicate
	PredIdent2 is round(PredIdent),
	Predicate = [ PredIdent2 , Relation , PredExpr ],
	% PredIdent is used to ident the predicate in pog
	% Relation is used to group predicates of a relation
	% PredExpr is the predicate itself used for predcounts query
	debug_writeln(build_list_of_predicates3/3, 59, [ 'predicate id ', PredIdent, ' is transformed to ', PredIdent2]),
	(
		not(optimizerOption(joinCorrelations)),
		not(optimizerOption(joinCorrelations2)),
		is_list(Relation)
		->
		info_continue(
			build_list_of_predicates3/2,
			[ 'join predicates not activated - ignore predicate ',
			Predicate ])
		;
		% add Relation to the listOfRelations
		add_relation(Relation),
		% add Predicate to the listOfPredicates
		add_predicate(Predicate),
		debug_writeln(build_list_of_predicates3/2, 51, [ 'relation ', Relation, ' and predicate ', Predicate, ' was added'])
	),
	!.

% unexpected call
build_list_of_predicates3(A, B):-
	error_exit(build_list_of_predicates3/2, [ 'unexpected call with arguments ', [ A, B ] ]).



/*
2.4.5 get\_predicate\_contents/3

  The predicate get\_predicate\_contents/3 returns the content
describing the predicate.

*/

% predicates using just one relation
get_predicate_contents(Predicate, PredExpr, Relation):-
	% only select supports selects only currently
	Predicate = select( _ , PredExpr ),
	PredExpr = pr( _ , Relation ),
	debug_writeln(get_predicate_contents/3, 61, [ 'select predicate evaluated ', Predicate, ' => ', [PredExpr, Relation]]).

% join predicates
get_predicate_contents(Predicate, PredExpr, Relation):-
	Predicate = join( _, _, PredExpr ),
	PredExpr = pr( _ , Relation1, Relation2 ),
	Relation = [ Relation1, Relation2 ],
	debug_writeln(get_predicate_contents/3, 62, [ 'join predicate evaluated ', Predicate, ' => ', [PredExpr, Relation]]).

% other unsupported type of predicate
get_predicate_contents(Predicate, _, _):-
	info_continue(get_predicate_contents/3, [ 'unsupported type of predicate ', Predicate, ' - ignore it']),
	fail.



/*

  //characters [1] Type: [\underline{\it ]  [}]

*/


/*
3 Part two - construct all predcount execution plans

3.1 The goal of Part two

  The goal of part two is the construction of execution plans of
predcounts queries, ???which are able to run against
secondo database. They are constructed using
predicates "listOfPredicates/1"[1] and "listOfRelations/1" created by part one.
Finally the part three will using the execution plans to collect statistics
to bring??? the POG more detailed.

3.2 The algorithm in few words

  1 *???Punkt sinnlos???*
    The used sql predicates and relations are stored as argument 1 of
    predicates listOfPredicates/1 and listOfRelations/1. (see part one)

  2 for each relation instance: creation of predcount execution plan

    1 collect all sql predicates using one relation instance

    2 construct predcount execution plan (one per relation instance)

    3 generate the list of POG identifiers of sql predicates used 
      by the constructed predcount execution plan

*/

/*
3.2.1 predcount execution plans

  The stream consuming sql operator predcount called with a list of 
sql predicates determine for each element of the stream which 
combination of given sql predicates are solved.
Each sql predicate i given as parameter is identified by an identifier
2\^i. This way of identifing allows to identify each combination of sql
predicates just by sum the identifiers of each solved sql predicate.
The result of the predcount operator is a stream of tuples. The count
of tuples is 2\^(count of sql predicates). Each tuple consists of an 
identifier identifing a unique comination of solved sql predicates and
number of tuples of the original stream solving this combination of 
sql predicates.

  The identifier does not correspond with the POG identifier 
of the sql predicate. (see section below)

*/
/*
3.2.2 POG identifiers

  Each sql predicate has gotten an identifier in POG during parsing 
the sql query.
To assign the collected statistics to the right edge of the POG,
it's necessary to translate each identifier of the results of predcount
to the corresponding identifier of the POG. This is done by predicate
translate\_ident\_bits/3. (see part three for details)
Part two supplies the translation by bundling the list of original POG 
identifiers to each constructed predcount execution plan.

*/

/*
3.3 The defined dynamic facts

none

*/
/*
3.4 The implementation of the predicates

3.4.1 build\_queries/1

  The predicate is the entry point of part two. The result is returned
as the argument. The result is a list of tuples. One tupel per
relation instance is created. Each tuple contains a query (second
element) which represents the predcount execution plan for that 
relation instance. The first element of the tupel is the list of 
predicate identifiers called translation list. (see section 3.2.2
above) The elements of the translation list are used in step 3 to 
translate the tupels responsed by predcount for the POG.

*/

build_queries(Queries4):-
	% get list of relations created in step 1
	listOfRelations(RelationList),
	% build queries
	build_queries(RelationList, Queries),
	debug_writeln(build_queries/1, 39, [ 'first list of queries built (contain redundant evaluations)', Queries ]),
	% remove obsolete queries
	% joining relations by product is default and can also happen if Option joinCorrelation2 is activated
	% (if no set of seful join predicates could be found)
	remove_obsolete_queries(Queries, [], [], QueriesRev),
	remove_obsolete_queries(QueriesRev, [], [], Queries4),
	debug_writeln(build_queries/1, 31, [ 'final list of queries built (without redundant evaluations)', Queries4 ]).

% unexpected call
build_queries(A):-
	error_exit(build_queries/1, [ 'unexpected call with argument ', A ]).

/*
3.4.2 remove\_obsolete\_queries/4

  % VJO doku

  While using Option correlationsProduct the optimizer calculates
the predcount results for joined relations. In contrast to the
Option correlationsJoin there is no need to explicitly calculate
the predcount results for singleton relations. That's why the
queries for singleton relations can be removed if the singleton
relation is a member of any set of joined relations of another
predcount query.

  Which relation will be removed depends on the order of relations
stored as fact listOfRelations/1.

*/

% end of recursion
remove_obsolete_queries([], _, QueryList, QueryList):-
	debug_writeln(remove_obsolete_queries/4, 41, [ 'end of recursion ', [[], 'unknown', QueryList, QueryList] ]).

% obsolete query -> remove query
remove_obsolete_queries([ QueryDesc | RQuery ], Keep, RQueryRev, QueryList):-
	QueryDesc = [ QueryPreds, _, _],
	subset(QueryPreds, Keep),
	debug_writeln(remove_obsolete_queries/4, 42, [ 'query ', QueryDesc, ' removed' ]),
	remove_obsolete_queries(RQuery, Keep, RQueryRev, QueryList).

% query still neseccary -> keep query
remove_obsolete_queries([ QueryDesc | RQuery ], Keep, RQueryRev, QueryList):-
	QueryDesc = [ QueryPreds, _, _],
	(
		not(is_join_list(QueryPreds))
		->
		merge_set(QueryPreds, Keep, KeepNew) ;
		KeepNew = Keep
	),
	debug_writeln(remove_obsolete_queries/4, 43, [ 'query ', QueryDesc, ' kept' ]),
	remove_obsolete_queries(RQuery, KeepNew, [ QueryDesc | RQueryRev ], QueryList).

% unexpected call
remove_obsolete_queries(A, B, C, D):-
	error_exit(remove_obsolete_queries/4, ['unexpected call with arguments ',[A, B, C, D]]).

/*
3.4.3 is\_join\_list/1

  The identifier of join predicates are enclosed by []. The
predicate looks for such elements within a list. If such one
is detected the list is identified as a lists containing
identifiers of join predicates.

*/

% join´predicate is found -> end of recursion
is_join_list([ [ J ] | _ ]):-
	debug_writeln(is_join_list/1, 51, [ 'join predicate id ', [J], 'detected' ]).
% current element isn't a identifier of a join predicate -> continue with next one
is_join_list([ NJ | IdentList]):-
	debug_writeln(is_join_list/1, 59, [ 'predicate id ', NJ, ' is no join']),
	is_join_list(IdentList).
	

/*
3.4.2 build\_queries/2

  The predicate implements a loop over all relation instances given
as first argument.

*/

% end of recursion
build_queries([], []):-
	debug_writeln(build_queries/2, 42, [ 'all queries built' ]).

build_queries([ Relation | RRest], [ Query | QRest]):-
	debug_outln('build_queries 2: ', Relation),
	% store all Predicates as facts corrPredicate/3
	deleteCorrPredicates,
	listOfPredicates(Predicates),
	storeCorrPredicates(Predicates),
	% build tupel for relation Relation
	build_query(Relation, Query),
	debug_writeln(build_queries/2, 41, [ 'query', Query, ' built for relation ', Relation ]),
	% recursive call for other relations in list
	build_queries(RRest, QRest),
	% remove temporary facts corrPredicate/3
	deleteCorrPredicates.

% unexpected call
build_queries(A, B):-
	error_exit(build_queries/2, ['unexpected call with arguments ', [A, B]]).

% delete temporary facts
deleteCorrPredicates:-
	retractall((corrPredicates(_, _, _))),
	debug_writeln(deleteCorrPredicates/0, 51, ['facts corrPredicate/3 removed']).

% unexpected call
deleteCorrPredicates:-
	error_exit(deleteCorrPredicates/0, ['unexpected call']).

% creates temporary facts
storeCorrPredicates([]):-
	debug_writeln(storeCorrPredicates/1, 52, [ 'all facts corrPredicates/3 stored' ]).

storeCorrPredicates([ [ PredID, PredRel, PredExpr ] | ListOfPredicates ]):-
	assert((corrPredicates(PredID, PredRel, PredExpr))),
	debug_writeln(storeCorrPredicates/1, 51, [ 'fact ', corrPredicates(PredID, PredRel, PredExpr), 'stored']),
	storeCorrPredicates(ListOfPredicates).

% unexpected call
storeCorrPredicates(A):-
	error_exit(storeCorrPredicates/1, ['unexpected call with argument ', A]).

/*
3.4.3 build\_query/2

  The predicate constructs the tuple [ PredIdentList Query ] 
for the given relation (argument 1).

*/

build_query(Relation, [ PredIdentList , Query , Relation ]):-
%?%build_query(Relation, [ PredIdentList , Query , RelPred ]):-
%?%	set_equal(Relation, RelPred), unklar wieso - 
	% build sel and join lists
	get_join_predicates(Relation, JoinPredicateIDs, JoinPredicates),
	debug_writeln(build_query/2, 59, [ 'got join predicates ', JoinPredicateIDs, ' for relation ', Relation]),
	get_sel_predicates(Relation, SelPredicateIDs, _),
	debug_writeln(build_query/2, 59, [ 'got select predicates ', SelPredicateIDs, ' for relation ', Relation]),
	!,
	% build relation expression
	build_relation_expr(Relation, JoinPredicates, RelationExpr, UsedJoinPredicateIDs),
	debug_writeln(build_query/2, 59, [ 'relation expression ', RelationExpr, ' built for relation ', Relation, ' using join predicates ', UsedJoinPredicateIDs]),
	!,
	% build list of predicate identifiers identifying the predicates
	% evaluated by predcount operator
	flatten(UsedJoinPredicateIDs, UsedJoinPredicateIDsTmp),
	flatten(JoinPredicateIDs, JoinPredicateIDsTmp),
	subtract(JoinPredicateIDsTmp, UsedJoinPredicateIDsTmp, UnUsedJoinPredicateIDs),
	append(SelPredicateIDs, UnUsedJoinPredicateIDs, PCPredicates),
	debug_writeln(build_query/2, 59, ['predcount expression to be built for predicates ', PCPredicates]),
	!,
	% build predcount expression
	build_predcount_exprs(PCPredicates, PredcountExpr, PredIdentListTmp), 
	debug_writeln(build_query/2, 59, ['predcount expression built ', PredcountExpr, ' with translation list ', PredIdentListTmp]),
	% the identifiers of used join predicates are added to
	% the list of predicate identifiers to support calculate_pog_ident/3
	% (the idents of used join preds are enclosed by [])
	append(PredIdentListTmp, UsedJoinPredicateIDs, PredIdentList),
	debug_writeln(build_query/2, 59, ['final tanslation list is ', PredIdentList]),
	!,
	% concatinate string
	atom_concat('query ', RelationExpr, PredcountExpr,
		' consume;', '', Query),
%?%	debug_writeln(build_query/2, 51, [ 'query tuple ', [ PredIdentList , Query , RelPred ], ' built for relation ', Relation]).
	debug_writeln(build_query/2, 51, [ 'query tuple ', [ PredIdentList , Query , Relation ], ' built for relation ', Relation]).


% unexpected call
build_query(A, B):-
	error_exit(build_query/2, ['unexpected call with arguments ', [A,B] ]).



/*
3.4.4 build\_relation\_expr/2

  The predicate builds relation expression " query relation feed \{alias\} " .
Setting the alias is necessary to support direct using of sql predicates
of the original query. 

  If Option correlationsProduct is activated the string "relation feed \{
alias \} is generated for each relation of a set of relations. The 
generated strings are concatinated and joined with the key words query and
product.

  If Option correlationsJoin is activated a more complicated
algorithm is used to produce a derivated relation used to
calculate the resulats of operator predcount. The algorithm
is described in capture ???VJO.

*/

% if Option correlationsJoin is set - applicate operator predcount on a derivated relation
build_relation_expr(Relation, JoinPredicates, RelationExpr, SmallestJoinSet):-
        optimizerOption(joinCorrelations2), % nicht Option Kreuzprodukt
	% if no join predicate exists use product by default
	not(JoinPredicates=[]),
%?%	is_list(Relation), wie sollte ein join pred definiert werden ueber eine relationsinstanz
	%!, no cut here - it is possible, that no set of join predicates joins all relations
	% in that case get_all_join_sets/3 fails (that's correct) and a product of relations should be used
	(
		get_all_join_sets(Relation, JoinPredicates, JoinSets),
		not(JoinSets=[])
		->
		debug_writeln(build_relation_expr/4, 69, [ 'set of possible sets of join predicates for relation ', Relation, ' is ', JoinSets]);
		debug_writeln(build_relation_expr/4, 69, [ 'no set sufficient join predicates for relation ', Relation, ' found - use product']),
		fail
	),
	!, 
	get_smallest_join(JoinSets, SmallestJoinSet, _),
	debug_writeln(build_relation_expr/4, 69, [ 'smallest set of join predicates for relation ', Relation, ' is ', SmallestJoinSet]),
	!,
	get_cheapest_path(SmallestJoinSet, CheapestJoinPath),
	debug_writeln(build_relation_expr/4, 69, [ 'cheapest path of smallest join ', CheapestJoinPath]),
	!,
	plan(CheapestJoinPath, CheapestJoinPlan),
	debug_writeln(build_relation_expr/4, 69, [ 'plan of cheapest path of smallest join ', CheapestJoinPlan]),
	!,
	plan_to_atom(CheapestJoinPlan, RelationExpr),
	debug_writeln(build_relation_expr/4, 61, [ '(Option joinCorrelations2 set) relation expression ', RelationExpr, ' built for relation ', Relation, ' using set of join predicates ', SmallestJoinSet]),
	!.

% if Option correlationsProduct is set or
% if Option correlationsJoin is set and no sufficient set of join predicates were found to build the derivated relation
%             - applicate operator predcount on a product of relations
% end of recursion
build_relation_expr([ LastRelation | [] ], _, RelationExpr, []):-
	(
		optimizerOption(joinCorrelations2);
		optimizerOption(joinCorrelations)
	),
	LastRelation = rel(_, _),
%?%	!, % keine Wirkung von unexpected call
	plan_to_atom(LastRelation, RelName), % determine relation
	% ??? add using of small relation / samples here
	% build alias if necessary
	build_alias_string(LastRelation, RelAliasString),
	atom_concat(' ', RelName, ' feed ', RelAliasString,
		' ', RelationExpr), % concatinate string
	debug_writeln(build_relation_expr/4, 62, [ '(Option joinCorrelations* set) last relation ', LastRelation, ' added as ', RelName, ' with alias ', RelAliasString, ' to product']).

% recursion
build_relation_expr([ Relation | OtherRelations ], JoinPredicates, RelationExpr, JoinPredicates2):-
	(
		optimizerOption(joinCorrelations2);
		optimizerOption(joinCorrelations)
	),
	Relation = rel(_, _),
%?%	!, % keine Wirkung von unexpected call
	plan_to_atom(Relation, RelName), % determine relation
	% ??? add using of small relation / samples here
	% build alias if necessary
	build_alias_string(Relation, RelAliasString),
	build_relation_expr(OtherRelations, JoinPredicates, RelationExprTmp, JoinPredicates2),
	atom_concat(RelationExprTmp, RelName, ' feed ', RelAliasString,
		' product ', RelationExpr), % concatinate string
	debug_writeln(build_relation_expr/4, 62, [ '(Option joinCorrelations* set) relation ', Relation, ' added as ', RelName, ' with alias ', RelAliasString, ' to product']).

% for singleton relations
build_relation_expr(Relation, [], RelationExpr, []):-
	Relation = rel(_, _),
%?%	!, % keine Wirkung von unexpected call
	plan_to_atom(Relation, RelName), % determine relation
	% ??? add using of small relation / samples here
	% build alias if necessary
	build_alias_string(Relation, RelAliasString),
	atom_concat(' ', RelName, ' feed ', RelAliasString, 
		'', RelationExpr), % concatinate string
	debug_writeln(build_relation_expr/4, 63, [ 'singleton relation ', Relation, ' used as ', RelName, ' with alias ', RelAliasString]).

% unexpected call
build_relation_expr(A, B, C, D):-
	error_exit(build_relation_expr/4, ['unexpected call with arguments ',[A, B, C, D]]).




/*
3.4.5 build\_alias\_string/2

  The predicate builds the relation alias expression " \{alias\} " .

*/

build_alias_string(rel(_,*), ''):-
	Relation = rel(_,*),
	debug_writeln(build_alias_string/2, 71, [ 'no alias given for relation ', Relation]). % no alias necessary

build_alias_string(rel(_,RelAlias), AliasString):-
	Relation = rel(_,RelAlias),
        % create alias expression
	atom_concat(' {', RelAlias, '}', '', '', AliasString),
	debug_writeln(build_alias_string/2, 72, [ ' alias given for relation ', Relation, ' is ', RelAlias]).

% unexpected call
build_alias_string(A, B):-
	error_exit(build_alias_string/2, ['unexpected call with arguments ',[A, B]]).



/*
3.4.6 build\_predcount\_exprs/3

  The predicate builds the predcounts expression string 
 " predcounts [ ... ] " .

*/

build_predcount_exprs(Predicates, PredcountString, PredIdentList):-
	% get list of predicates created by part one
	findall(
		[PredID, Predicate], 
		(
			corrPredicates(PredID, _, Predicate),
			member(PredID, Predicates)
		), 
		ListOfPredicates
	),
	debug_writeln(build_predcount_exprs/3, 69, [ 'list of predicates ', ListOfPredicates, ' for predcount expression ']),
%?%	!, wegen unexpected call raus
	% build list of predcounts expression elements and 
	% predicate translation list
	build_predcount_expr(ListOfPredicates, PredcountExpr, PredIdentList),
	atom_concat(' predcounts [ ', PredcountExpr, ' ] ', '', '', 
		PredcountString),
	debug_writeln(build_predcount_exprs/3, 61, [ 'predcount expression ', PredcountString, ' and translation list ', PredIdentList, ' for list of predicates ',ListOfPredicates]).

% unexpected call
build_predcount_exprs(A, B, C):-
	error_exit(build_predcount_exprs/3, [ 'unexpected call with arguments ',[A, B, C]]).


/*
3.4.7 build\_predcount\_expr/4

  The predicate builds the expression for each sql predicate of
predcounts expression string and concates the expression to the list
of predcount expressions (argument 3). It adds the POG identifier 
of the sql predicate to the translation list (argument 4), too.


*/

build_predcount_expr([], '', []):- % end of recursion
	debug_writeln(build_predcount_expr/3, 72, ['end of recursion']).
build_predcount_expr([ Predicate | Rest ], PredcountExpr, 
	PredIdentList):- % used if Predicate use Relation
	Predicate = [ PredIdentNew, PredExprNew ],
	get_predicate_expression(PredIdentNew, PredExprNew, PredExpr, PredIdent),
%nur chaos	debug_writeln(build_predcount_expr/3, 79, [ 'using ', [PredIdent,PredExpr], ' for ', [PredIdentNew, PredExprNew]]),
%?%	!, no longer necessary, because of using of get_predicate_expression/4
	% recursive call for other predicates in list
	build_predcount_expr(Rest, PERest, PIRest),
	%!, % if Predicate does not use Relation add_predcount fails
	build_predicate_expression(PredIdent, PredExpr, 
		PredPredcountExpr), 
	debug_writeln(build_predcount_expr/3, 79, [ 'predcount expression ', PredPredcountExpr, ' for predicate ', Predicate]),
	build_comma_separated_string(PredPredcountExpr, PERest, 
		PredcountExpr),
	debug_writeln(build_predcount_expr/3, 71, [ 'new predcount list expression ', PredcountExpr]),
	% add predicate identifier of POG to list 
	PredIdentList = [ PredIdent | PIRest ],
	debug_writeln(build_predcount_expr/3, 71, [ 'new translation list ', PredIdentList]).

% unexpected call
build_predcount_expr(A, B, C):-
	error_exit(build_predcount_expr/3, ['unexpected call',[A, B, C]]).


/*
3.4.8 get\_predicate\_expression/2

	The predicate get\_predicate\_expression/2 returns the
predicates expression.

*/

% predicates using one relation
get_predicate_expression(Ident, Predicate, PredExpr, Ident):-
	Predicate = pr( PredExpr, _),
	debug_writeln(get_predicate_expression/4, 81, [ 'expression for selection predicate ', PredExpr]).
% predicates using two relations (join)
get_predicate_expression(Ident, Predicate, PredExpr, Ident):-
	Predicate = pr( PredExpr, _, _),
	debug_writeln(get_predicate_expression/4, 82, [ 'expression for join predicate ', PredExpr]).
%X01get_predicate_expression(Ident, Predicate, '', [Ident]):-
%X01	Predicate = pr( _, _, _).
% unexpected call
get_predicate_expression(A, B, C, D):-
	error_exit(get_predicate_expression/4, 
		['unexpected call (only predicates of form pr/2 and pr/3 supported)',
		[A, B, C, D]]).

/*
3.4.8 build\_predicate\_expression/3

  The predicate constructs one sql predicate expression for predcount
expression.

*/

% used for predicates which should not be evaluated by predcount operator
%X01build_predicate_expression(_, '', ''):-
%X01	debug_outln('build_predicate_expression (no expression)').

build_predicate_expression(PredIdent, Expression, PredcountExpr):-
	plan_to_atom(Expression, PredExpr),
	atom_concat('P', PredIdent, ': ', PredExpr,
		'', PredcountExpr),
	debug_writeln(build_predicate_expression/3, 81, [ 'built predcount expression ', PredcountExpr, ' for predicate ', [PredIdent, Expression]]). 

% unexpected call
build_predicate_expression(A, B, C):-
	error_exit(build_predicate_expression/3, [ 'unexpected call with arguments ',[A, B, C]]).



/*
3.4.9 build\_comma\_separated\_string/3

  Arg3 = Arg1 + ',' + Arg2 

*/

build_comma_separated_string('', AtEnd, AtEnd):-
	debug_writeln(build_comma_separated_string/3, 81, [ 'arguments ', ['', AtEnd], ' result ', AtEnd]).
build_comma_separated_string(AtBegin, '', AtBegin):-
	debug_writeln(build_comma_separated_string/3, 81, [ 'arguments ', [AtBegin, ''], ' result ', AtBegin]).
build_comma_separated_string(AtBegin, AtEnd, NewString):-
	atom_concat(AtBegin, ',', AtEnd, '', '', NewString),
	debug_writeln(build_comma_separated_string/3, 81, [ 'arguments ', [AtBegin, AtEnd], ' result ', NewString]).
build_comma_separated_string(A, B, C):-
	error_exit(build_comma_separated_string/3, [ 'unexpected call with arguments ',[A, B, C]]).


/*
4 Part three - execution of predcounts plans and 
tranforming of the results for the POG

4.1 Goal of part three

  After the construction of predcounts execution plans
in part two the plans
have to be executed now. The predcounts results have to be transformed
because
the identifiers used by operator predcounts don't generally match with the
identifiers used by POG. Finally the results are added to the POG.

4.2 Algorithm in few words

  1 execute predcounts execution plan

  2 transform the predicate identifiers (atom) of the operator predcounts
    to the correspondig node identifiers used by the POG 

4.3 Defined dynamic facts

  none

4.4 Implementation of the predicates

4.4.1 calculate\_predcounts/2

  The predicate is the entry point of part three. It uses the result of
part two as first argument and returns the result as second arguent.
It executes any given predcounts execution plan and transforms each predicate
identifiers of operator predcounts to POG's one using
translate\_ident\_bits.

*/

% end of recursion
calculate_predcounts([], []):-
	retractall(predcounts_result(_,_)),
	debug_writeln(calculate_predcounts/2, 32, [ 'all facts predcounts_result/2 removed' ]).
calculate_predcounts([[PredIDs , Query , Relation ] | QRest],
	[PredCount2 | PRest]):-
	% rekursiv alle predcounts-Plaene betrachten
	calculate_predcounts(QRest,PRest),
	!,
	% predcounts-Plan ausfuehren
	secondo(Query, [_|[PredTupels]]),
	debug_writeln(calculate_predcounts/2, 39, [ 'predcounts result for relations ', Relation, ' is ', PredTupels]),
	!,
	% Wert fuer atom in Ergebnistupel in den Wert 
	% fuer POG uebersetzen
	translate_ident_bits(PredTupels, PredIDs, PredCount),
	debug_writeln(calculate_predcounts/2, 39, [ 'translated predcounts results ', PredCount ]),
	% bei Nutzung von nested-loop statt product ist noch ein Tupel aufzunehmen
	(
		optimizerOption(joinCorrelations2), % nicht Kreuzprodukt
		%containsJoinPred(PredIDs)
		% Relation is a list of Relations
		is_list(Relation)
		->
		debug_writeln(calculate_predcounts/2, 39, [ 'add number of tuples of product of ', Relation, ', which do not meet the join predicate']),
		get_card_product(Relation, CardProduct),
		debug_writeln(calculate_predcounts/2, 39, [ 'cardinality of product of all relations ', Relation, ' is ', CardProduct]),
		get_sum_tuples(PredCount, SumTuples),
		debug_writeln(calculate_predcounts/2, 39, [ 'sum of all predcount tupels ', PredCount, ' is ', SumTuples]),
		PCJoin is CardProduct - SumTuples,
		debug_writeln(calculate_predcounts/2, 39, [ 'number of tuples of product of ', Relation, ', which do not meet the join predicate is ', PCJoin]),
		PredCount2 = [ [ 0 , PCJoin ] | PredCount ];
		PredCount2 = PredCount
	),
	% speichere predcounts Ergebnisse als Fakt
	assert(predcounts_result(Relation, PredCount2)),
	debug_writeln(calculate_predcounts/2, 31, [ 'store relations ', Relation, ' predcounts result ', PredCount2, ' as fact']).
% unexpected call
calculate_predcounts(A, B):-
	error_exit(calculate_predcounts/2, ['unexpected call with arguments ',[A, B]]).

/*
4.4.2 Helper get_sum_tuples/2

  VJO Docu ???

*/

% end of recursion
get_sum_tuples([], 0):-
	debug_writeln(get_sum_tuples/2, 42, ['end of recursion']).
get_sum_tuples([ [ _ , Count] | Rest ], SumTuplesNew):-
	get_sum_tuples(Rest, SumTuplesOld),
	SumTuplesNew is SumTuplesOld + Count,
	debug_writeln(get_sum_tuples/2, 41, [Count, ' added to ', SumTuplesOld, ' is ', SumTuplesNew]).
% unexpected call
get_sum_tuples(A, B):-
	error_exit(get_sum_tuples/2, ['unexpected call',[A, B]]).

/*
4.4.2 Helper get_card_product/2

  VJO Docu ???

*/

get_card_product([], 0):-
	debug_writeln(get_card_product/2, 42, [ 'end of recursion']).
get_card_product(RelationList, CP):-
	get_card_product2(RelationList, CP),
	debug_writeln(get_card_product/2, 41, ['product of cardinalities of ', RelationList, ' is ', CP]).
get_card_product(Relation, Card):-
	get_card_relation(Relation, Card),
	debug_writeln(get_card_product/2, 43, ['cardinality of ', Relation, ' is ', Card]).
% unexpected call
get_card_product(A, B):-
	error_exit(get_card_product/2, ['unexpected call',[A, B]]).	

% end of recursion
get_card_product2([], 1):-
	debug_writeln(get_card_product2/2, 52, [ 'end of recursion']).
get_card_product2([ Relation | Rest ], CPNew):-
	get_card_relation(Relation, Card),
	get_card_product2(Rest, CPOld),
	CPNew is CPOld * Card,
	debug_writeln(get_card_product2/2, 51, ['multi old product of cardinalities ', CPOld, ' with ', Card, ' to ', CPNew]).
% unexpected call
get_card_product2(A, B):-
	error_exit(get_card_product2/2, ['unexpected call',[A, B]]).
/*
4.4.2 Helper get_card_relation/2

  VJO Docu ???

*/

get_card_relation(Relation, Card):-
	get_relationid(Relation, RelationID),
	resSize(RelationID, Card),
	debug_writeln(get_card_relation/2, 61, ['cardinality of ', Relation, ' with id ', RelationID, ' is ', Card]).

get_relationid(Relation, RelationID):-
	node(0, _, RelationList),
	get_relationid2(Relation, RelationList, RelationID),
	debug_writeln(get_relationid/2, 71, ['relation id of relation ', Relation, ' is ', RelationID]).

% end of recursion
get_relationid2(Relation, [ RelDesc | _], RelationID):-
	RelDesc = arp( RelationID, [ Relation ], _),
	debug_writeln(get_relationid2/3, 82, ['relation id of relation ', Relation, ' is ', RelationID]).
get_relationid2(Relation, [ _ | Rest], RelationID):-
	debug_writeln(get_relationid2/3, 81, ['search again in list ', Rest]),
	get_relationid2(Relation, Rest, RelationID).
% unexpected call
get_relationid2(A, B, C):-
	error_exit(get_relationid2/2, ['unexpected call',[A, B, C]]).

/*
4.4.2 translate\_ident\_bits/3

  The predicate transforms the predicate identifiers of the
operator predcounts 
(argument one) to the node identifiers of POG (argument three) using
the translation tables given as a list (argument two).
Each atom of argument one is finally translated by using 
calculate\_pog\_ident/3.

*/

% end of recursion
translate_ident_bits([], _, []). % Rekursionsabschluss
translate_ident_bits([ InTupel | IRest], TranTab,
	[ OutTupel | ORest]):-
	% rekursiv alle alle Tupel durchgehen
	translate_ident_bits(IRest, TranTab, ORest),
	% Tupel zerlegen
	InTupel = [ InAtom | Count ],
	% Wert fuer Atom in korrespondierenden POG-Wert uebersetzen
	calculate_pog_ident(InAtom, TranTab, OutAtom),
	% Zieltupel zusammensetzen
	OutTupel = [ OutAtom | Count],
	debug_writeln(translate_ident_bits/3, 41, [ 'real id of ', InAtom, ' is ', OutAtom, ' using translation list ', TranTab]).
% unexpected call
translate_ident_bits(A, B, C):-
	error_exit(translate_ident_bits/2, ['unexpected call',[A, B, C]]).


/*
4.4.3 calculate\_pog\_ident/3

  The predicate translate each atom identifier of the operator
predcounts to the corresponding node identifier of POG.

*/

% end of recursion
calculate_pog_ident(0, [], 0):- % Rekursionsabschluss
	debug_writeln(calculate_pog_ident/3, 52, ['end of recursion']).
calculate_pog_ident(InAtom, [ [ JoinId ] | TRest ], OutAtom):-
	debug_outln('calculate_pog_ident: add join ID ', JoinId),
	calculate_pog_ident(InAtom, TRest, OutAtom2),
	OutAtom is OutAtom2 + JoinId,
	debug_writeln(calculate_pog_ident/3, 53, ['add join id ', JoinId, ' to id => new id ', OutAtom]).
calculate_pog_ident(InAtom, [ TranElem | TRest ], OutAtom):-
	% mod = Ganzzahl; restliche Bits
	IRest is InAtom // 2,
	IBit is InAtom - ( 2 * IRest), 
	calculate_pog_ident(IRest, TRest, ORest), 
	OBit is IBit * TranElem,
	OutAtom is ORest + OBit,
	debug_writeln(calculate_pog_ident/3, 51, ['???VJO???']).
% unexpected call
calculate_pog_ident(A, B, C):-
	error_exit(calculate_pog_ident/2, ['unexpected call',[A, B, C]]).
/*
4 Implementation of interfaces for SecondoPL

4.1 Overview

*/

/*
4.4.24 feasible/4

	the predicate feasible/4 implemented in entropy\_opt.pl is designed
for the special purposes of entropy approach. The reimplemented feasible/4
is more general. It only assumes that the cardinality of a POG's node
will only be determineable if an approximation of the node was done before.
???

	The new implementation checks any node (except node 0) with a 
known cardinality approximation against all of it's previous nodes
with a known cardinality approximation.

	Any previous node $N_p,i \in \{ N_x \in N | P_{N_x} \subset P_{N_i} \wedge |P_{N_i} \setminus P_{N_x}| = 1\}$ of a node $N_i$ ...


*/

% orginal feasible geht von fester REihenfolge aus,
% dass ist aber nicht mehr gegeben
/* in constrast to the implementation of feasible there isn't an order of
edges within the list of joint selectivities */



































/*
5 A simple example

5.1 Overview

  The following simple example explains the using of

  * build\_list\_of\_predicates/0

  * build\_queries/1

  * calculate\_predcounts/2

5.2 Code example

  The output that shows the content of variables is restructured
for easier understanding. Normally the variable's content is shown
within one line per variable. For easier reading and referencing
such a line is devided into more than one. Additionally these lines
are numerated.

----
    F=[8,[2,3]]
----

  For example such a original prolog output line would
devided as follows:

----
    000 F=
    001 [
    002   8,
    003   [
    004     2,
    005     3
    006   ]
    007 ]
----

  Now it's possible to reference each element just by the
line's number.

  In Prolog all structured data is represented as a list.
For simplifing the explanations below we decide between a list and
a tuple. Both structures are represented as a list in Prolog, but
a tuple is a list with a fixed len. In no case a list called 
tuple will be composed of more or less elements as described in
this example.

  For example the list of predicates is a typical list
because their len is determined by the sql query to be optimized.
The predicate description which the list of predicates is composed of
is a typical tuple. It ever contains of three element.

5.2.1 Preparation

----
    1 ?- secondo('open database opt;').
    Total runtime ...   Times (elapsed / cpu): 0.010001sec / 0.01sec = 1.0001
    Command succeeded, result:
    
    ** output truncated **

    2 ?- setOption(entropy).
    Total runtime ... Times (elapsed / cpu): 0.080001sec / 0.07sec = 1.14287
    Switched ON option: 'entropy' - Use entropy to estimate selectivities.

    Yes
----

  Setting the option entropy is nessecary first to let the
system generate the dynamic facts edge/6. These facts are used
by later prolog praedicates to analyse the sql query to be
optimized.

  After toggling on the Option entropy the sql query to be
optimized has to be entered.

----
    3 ?- sql select count(*) from [ plz as a, plz as b ]
     where [a:plz > 40000, b:plz < 50000, a:plz <50000].
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    Computing best Plan ...
    Destination node 7 reached at iteration 6
    Height of search tree for boundary is 1
    
    No
----

  After entering the sql query the option correlation
has to be toggled on. By doing this the option entropy
will be toggled off automatically.

----
    4 ?- setOption(correlations).
    Switched OFF option: 'entropy' - Use entropy to estimate selectivities.
    % ./Correlations/correlations compiled 0.01 sec, 10,660 bytes
    Switched ON option: 'correlations' - Try to take predicate interdependence into account.

    Yes
----

5.2.2 Analyze the query to be opimized

  This analysis is done by the prolog predicate 
build\_list\_of\_predicates/0.

----
    5 ?- build_list_of_predicates.

    No
----

  After calling the predicate build\_list\_of\_predicates/0
the dynamic facts listOfPredicates/1 and listOfRelations/1 were
created. The content of their arguments are shown below.

----
    6 ?- listOfPredicates(X).
    010 X = 
    020 [
    030   [
    031     4, 
    032     rel(plz, a), 
    033     pr(attr(a:pLZ, 1, u)<50000, rel(plz, a))
    034   ], 
    040   [
    041     2, 
    042     rel(plz, b), 
    043     pr(attr(b:pLZ, 1, u)<50000, rel(plz, b))
    044   ], 
    050   [
    051     1, 
    052     rel(plz, a), 
    053     pr(attr(a:pLZ, 1, u)>40000, rel(plz, a))
    054   ]
    060 ]
    
    Yes
----

  The content of X is the list of sql predicates found by
predicate build\_list\_of\_predicates/0 before. Each sql predicate found 
is represented by one tuple (lines 30-34, 40-44 and 50-54).
Each tuple is composed of three elements: (sql joins are disallowed yet!)

  1 POG identifier of the sql predicate (lines 31,41 and 51)

  2 the relation instance of the sql predicate represented by fact rel/2
(lines 32, 42 and 52)

  3 the sql predicate itself (lines 33, 43 and 53)

----

    7 ?- listOfRelations(Y).
    
    110 Y = 
    120 [
    130   rel(plz, b, l), 
    140   rel(plz, a, l)
    150 ]
    
    Yes
----

  The content of Y is the distinctive list of relation instances
found by build\_list\_of\_predicates/0. Although two sql predicates use
the same relation PLZ the relation was added twice to the list
because the relation PLZ was used as relation instance a and relation
instance b within the sql query. On the other hand the relation instance
b was added just once, although the relation instance is used by
two predicates. For a more detailed explanation of relation instances
see the section instances of relations above.

5.2.3 Build predcounts queries and get the statistics

----
    8 ?- build_queries(Queries), calculate_predcounts(Queries,Result).
    0.init finished
    Total runtime ... Times (elapsed / cpu): 1.91003sec / 1.91sec = 1.00002
    0.init finished
    Total runtime ... Times (elapsed / cpu): 0.530008sec / 0.54sec = 0.981496
    210 Queries = 
    220 [
    230   [
    231     [2] |
    232     query plz feed {b} predcounts [ P2: (.PLZ_b < 50000) ]
    232     consume;
    233   ], 
    240   [
    241     [4, 1] |
    242     query plz  feed  {a} predcounts [ P4: (.PLZ_a < 50000),
    242     P1: (.PLZ_a > 40000) ]  consume;
    243   ]
    250 ],
    310 Result = 
    320 [
    330   [
    331     [2, 21470], 
    332     [0, 19797]
    333   ], 
    340   [
    341     [5, 3127], 
    342     [1, 19797], 
    343     [4, 18343], 
    344     [0, 0]
    345   ]
    350 ]

    Yes
----

  The variable Queries contains a list of tuples (lines
230-233 and 240-243). One tuple
corresponds with exactly one relation instance (lines 130
and 140)
found by build\_list\_of\_predicates/0. Each tuple is composed of two 
elements:

  1 the list of POG identifiers of the sql predicates used by
the predcounts execution plan (see element two);  the order of the
POG identifiers corresponds with the order of using the sql predicates
within the predcounts execution plan

  2 predcounts execution plan of one relation instance with all
sql predicates using this relation instance

  The variable Result is unified with the list of results of
predcounts execution plans created by the predicate build\_queries/1.
(lines 232 and 242) Each result corresponds with one relation instance.
The order of predcounts results stored in Result matches the order of
predcounts execution plans in the variable Queries.

  The predcounts result is a list of tuples. Each tuple is
composed of a element atom (first one) and an element count (second
one). An explanation of the tuple's meaning could be found in the 
documentation of operator predcounts.

  The predcounts's results stored in Result are already
transformed. What means transformed? This issue is discussed later in
this document detailed. In a short manner one could say, the identifiers
of the tuples (column atom) are transformed for using it directly to
identify nodes of the POG.

5.2.4 How are the predcounts results to be interpreted ?

  Each predcounts result is exclusively calculated for
exact one relation instance. The tuples attribute atom identifies which
predicates a tuple has to solve and not to solve to increase the count value
of this tuple. 

  To understand the meaning of predcounts result the predcounts
execution plan of relation instance a (line 242) will be executed. 
  
----
    9 ?- secondo(query plz  feed  {a} predcounts [ P4: (.PLZ_a < 50000),
    P1: (.PLZ_a > 40000) ]  consume;', A).
    0.init finished
    Total runtime ...   Times (elapsed / cpu): 0.940014sec / 0.89sec = 1.0562
    
    410 A =
    420 [
    430   [rel, 
    431     [tuple, 
    432       [
    433         [Atom, int], 
    434         [Count, int]
    435       ]
    436     ]
    437   ],
    440   [
    441     [3, 3127],
    442     [2, 19797],
    443     [1, 18343],
    444     [0, 0]
    445   ]
    450 ]
    
    Yes
----

  While looking at the listing above you will see the predcounts
result is a list of tuples and each tuple is composed of a value Atom
and a value Count.

  To understand the value atom one has to imagine the value in
binary mode. So the value 1 is 01. The Count 18343 means that 18343 tuples
of the relation instance b solve the the predicate P4 but not the
predicate P1. (Please note the tuples must not solve the predicate P1.
This is in contradiction to the meaning of POG's nodes identifier.)

  To determine how many tuples of relation instance a solves
the predicate P4 in independence of predicate P1, the values Count of 
the result's tuples identified by 2 (binary 10) and 3 (binary 11) 
has to be added. In the example you get a count of 18343 + 3127 = 21470.

5.2.5 Some Theory

5.2.5.1 Definitions

	.

  Be $Pi = \{ p_1, p_2, p_4, ..., p_{2^m} \}$ the set of predicates used
by the sql query to be optimized and $P_{N_i}$ the set of predicates
solved at POG's node $N_i$.

  Be $R = \{R_0, R_1, \ldots, R_n \}$ the set of relation instances used by
the sql query to be optimized and $R_{N_i}$ the set of relation instances
used by the predicates of set $P_{N_i}$.

  Be $C_{N_0} = R_0 \times R_1 \times \ldots \times R_n$ the
initial set of tupels at the start node $N_0$.

  Be $r: P \rightarrow 2^R$ the function which determines the
relation instances $R_{p_i} \subseteq R$ used by the predicate
$p_i \in P$.

  Be the cardinality $C_{N_i}$ the count of tuples which solves all
predicates $p \in P_{N_i}$.

  Be the selectivity $S_{N_i} = \frac{C_{N_i}}{C_{N_0}}$.

  Be $PC_{R_i} \subset \aleph \times \aleph$ the predcounts result. Each 
element $t \in PC_{R_i}$ is composed of an attribute atom and an attribute
count. The elements of the tuple $t \in PC_{R_i}$ can be obtained
by the functions $a_{atom}: PC_{R_i} \rightarrow \aleph$ and
$a_{count}: PC_{R_i} \rightarrow \aleph$. In addition there is a function
$solved: PC_{R_i} \rightarrow P$ which determines the set of predicates
solved by the tuples $e \in R_i$ that are represented by tuple $t \in PC_{R_i}$
of predcounts result.

  Be $card: R_{N_i} \times \aleph \rightarrow \aleph$ a function which
determines $\forall t \in R_{N_i}, n \le 2^{|P|}: card(t,n) = ???$

  Be $CM_{R_i}$ the set of tuples $c_{R_i,\aleph} \in \aleph \times
\aleph$ of the the predcounts result. The elements of the tuple $c_{R_i,\aleph}$
can be obtained by the functions $a_{atom}(c_{R_i,\aleph}) \mapsto \aleph$ and
$a_{count}(c_{R_i,\aleph}) \mapsto \aleph$

  Be $s: PC_{R_i} \rightarrow P$ the function which determines
the set of predicates solved by tuples represented by a tuple
of predcounts result PC. For example the $a_{atom}(R_i,3)=5$ returns
the set $\{p_4, p_1\}$.

5.2.5.2 Types of POG's nodes

	.

  While evaluating the predcounts results it's possible to devide
the set of POG's nodes into the following types of nodes:

  ** was ist mit Praedikaten, die keine Relation verwenden **

	1. the start node $N_0$ with $|P_{N_0}| = 0$

	2. nodes $N_i$ with $|P_{N_i}| = 1$ and $|R_{N_i}| = 1$

	3. nodes $N_i$ with $|R_{N_i}| = 1$

	4. nodes $N_i$ with $|R_{N_i}| > 1$ but 
$\forall p \in P_{N_i}: |r(p)| = 1$

	5. nodes $N_i$ with $\exists p \in P_{N_i}: |r(p)| \ge 2$

5.2.5.3 Determine the cardinalities

	** Das folgende muesste eigentlich bewiesen werden - oder ? **

5.2.5.3.1 Node of type 1

	Within a POG there exist only one node of type 1. This node is
the start node of the POG.

	To determine the cardinality of node $N_0$ based on predcount results
the cardinalities $C_{R_i}$ of all relation instances have to calculated first.
This can be done by: $$C_{R_i} = \sum_t^{PC_{R_i}} a_{count}(t)$$ 

	The cardinality $C_{N_0}$ can be calculated as follows:
$$C_{N_0} = \prod_{R_i}^R C_{R_i}$$

5.2.5.3.1 Nodes of type 2 and 3

	The nodes of type 2 are all nodes which follows directly of the
POG's start node. At all of these nodes just one predicate was evaluated.
The predicate evaluated uses exactly one relation instance $R_{p_i} \in
R_{N_i}$ with $p_i \in P_{N_i}$.


	The cardinality of these nodes $C_{N_i}$ can be obtained using
predcounts results $PC_{R_i}$:
$$C_{N_i} = C_{R_i}' * \prod_{R_j}^{R \setminus R_{N_i}} C_{R_j}$$
with
$$C_{R_i}' = \sum_t^{\{e \in PC_{R_i} { | } \forall p \in P_{N_i}:
p \in solved(e)\}}
a_{count}(t)$$

	The way described above is also usable to determine the
cardinality $C_{N_i}$ of nodes of type 3.

5.2.5.3.1 Nodes of type 4

	The nodes of type 4 includes all nodes except type 1,2,3 nodes and
nodes which includes join predicates. To obtain the cardinality of such
a node the following equation has to be solved:
$$C_{N_i} = \prod_{R_m}^{R_{N_i}} C_{R_m}' * 
\prod_{R_j}^{R \setminus R_{N_i}} C_{R_j}$$ with
$$C_{R_m}' = \sum_t^{\{e \in PC_{R_m} { | } \forall p \in P_{N_i}:
p \in solved(e)\}} a_{count}(t)$$.

	The both equations above are the general form of the equations
for type 1,2 and 3 nodes.

5.2.5.3.1 nodes of type 5

	The nodes of type 5 are nodes including join predicates. The
source is not still able to deal with these predicates.

5.2.6 The cardinalities of the example's nodes

  The sql query to be optimized contains three predicates (see lines 010 - 060):

  * $P1: attr(a:pLZ, 1, u)>40000$

  * $P2: attr(b:pLZ, 1, u)<50000$

  * $P4: attr(a:pLZ, 1, u)<50000$

  The three predicates uses two relation instances. Therefore two
predcounts execution plans were created (lines 232 and 242).

  * $query plz feed \{b\} predcounts [ P2: (.PLZ\_b < 50000) ] consume;$

  * $query plz  feed  \{a\} predcounts [ P4: (.PLZ\_a < 50000), 
P1: (.PLZ_a > 40000) ]  consume;$

  The queries's result was stored in the variable Result as follows:

----
    310 Result =
    320 [
    330   [
    331     [2, 21470],
    332     [0, 19797]
    333   ],
    340   [
    341     [5, 3127],
    342     [1, 19797],
    343     [4, 18343],
    344     [0, 0]
    345   ]
    350 ]
----

  Using the predcounts result's above we get:

  * $R = \{ R_a, R_b \}$

  * $PC_{R_a} = \{ (5,3127), (1,19797), (4,18343), (0,0) \}$

  * $PC_{R_b} = \{ (2,21470), (0,19797) \}$

  By adding some values we get additionally:
$$C_{R_a} = \sum_t^{PC_{R_a}} a_{count}(t) = 3127 + 19797 + 18343 + 0 = 41267$$
$$C_{R_b} = \sum_t^{PC_{R_b}} a_{count}(t) = 21470 + 19797 = 41267$$

  At first all nodes of the example's POG are of the type 1 to 4.
So the cardinality of each POG's node can be obtained by using the
equations for type 4 nodes.


5.2.6.1 node $N_0$ (type 1)

	$P_{N_0} = \emptyset$ and $R_{N_0} = \emptyset$

  $$C_{N_0} = \prod_{R_i}^R C_{R_i} $$
$$ C_{N_0} =  \prod_{R_i}^{\{ R_a, R_b\}} C_{R_i} $$
$$ C_{N_0} =  C_{R_a} * C_{R_b} $$
$$ C_{N_0} = 41267 * 41267 $$
$$ C_{N_0} = 1702965289 $$

5.2.6.2 node $N_1$ (type 2)

	$P_{N_1} = \{ p_1 \}$ and $R_{N_1} = \{ R_a \}$

  $$ C_{N_1} = C_{R_{N_1}}' * \prod_{R_i}^{R \setminus R_{N_1}} C_{R_i}$$
$$ C_{N_1} = C_{R_a}' * \prod_{R_i}^{R \setminus \{ R_a \}} C_{R_i} $$
$$ C_{N_1} = C_{R_a}' * \prod_{R_i}^{\{ R_b \}} C_{R_i} $$
$$ C_{N_1} = C_{R_a}' * C_{R_b} $$
$$ C_{N_1} = \left( \sum_t^{\{e \in PC_{R_a} { | } \forall p \in \{ p_1 \}: 
p \in solved(e)\}} a_{count}(t) \right) * C_{R_b} $$
$$ C_{N_1} = \left( \sum_t^{\{ (5,3127), (1,19797) \}} a_{count}(t)
\right) * C_{R_b} $$
$$ C_{N_1} = ( 3127 + 19797 ) * 41267$$
$$ C_{N_1} = 946004708 $$

5.2.6.3 node $N_2$ (type 2)

	$P_{N_2} = \{ p_2 \}$ and $R_{N_2} = \{ R_b \}$

$$ C_{N_2} = C_{R_b}' * C_{R_a} $$
$$ C_{N_2} = \left( \sum_t^{\{e \in PC_{R_b} { | } \forall p \in \{ p_2 \}: 
p \in solved(e)\}} a_{count}(t) \right) * C_{R_a} $$
$$ C_{N_2} = \left( \sum_t^{\{ (2,21470) \}} a_{count}(t)
\right) * C_{R_a} $$
$$ C_{N_2} = ( 21470 ) * 41267$$
$$ C_{N_2} = 886002490 $$

5.2.6.4 node $N_4$ (type 2)

	$P_{N_4} = \{ p_4 \}$ and $R_{N_4} = \{ R_a \}$

$$ C_{N_4} = C_{R_a}' * C_{R_b} $$
$$ C_{N_4} = \left( \sum_t^{\{e \in PC_{R_a} { | } \forall p \in \{ p_4 \}: 
p \in solved(e)\}} a_{count}(t) \right) * C_{R_b} $$
$$ C_{N_4} = \left( \sum_t^{\{ (5,3127), (4,18343) \}} a_{count}(t)
\right) * C_{R_b} $$
$$ C_{N_4} = ( 3127 + 18343 ) * 41267$$
$$ C_{N_4} = 886002490 $$

5.2.6.4 node $N_5$ (type 3)

	$P_{N_5} = \{ p_1, p_4 \}$ and $R_{N_5} = \{ R_a \}$

$$ C_{N_4} = C_{R_a}' * C_{R_b} $$
$$ C_{N_4} = \left( \sum_t^{\{e \in PC_{R_a} { | } \forall p \in \{ p_1, p_4 \}:
p \in solved(e)\}} a_{count}(t) \right) * C_{R_b} $$
$$ C_{N_4} = \left( \sum_t^{\{ (5,3127) \}} a_{count}(t)
\right) * C_{R_b} $$
$$ C_{N_4} = ( 3127 ) * 41267$$
$$ C_{N_4} = 129041909 $$


5.2.6.5 node $N_3$ (type 4)

	$P_{N_3} = \{ p_1, p_2 \}$ and $R_{N_3} = \{ R_a, R_b \}$

  $$ C_{N_3} = \prod_{R_m}^{R_{N_3}} C_{R_m}' * 
\prod_{R_j}^{R \setminus R_{N_3}} C_{R_j} $$
$$ C_{N_3} = \prod_{R_m}^{\{R_a, R_b\}} C_{R_m}' * 
\prod_{R_j}^{R \setminus \{ R_a, R_b \}} C_{R_j} $$
$$ C_{N_3} = \prod_{R_m}^{\{R_a, R_b\}} C_{R_m}' * 
\prod_{R_j}^{\emptyset} C_{R_j} $$
$$ C_{N_3} = \prod_{R_m}^{\{R_a, R_b\}} C_{R_m}'$$
$$ C_{N_3} = C_{R_a}' * C_{R_b}' $$

  ** aus nachfolgender Formel ergibt sich noch ein Fehler (p in Durchschnitt!) **

$$ C_{N_3} = \sum_t^{\{e \in PC_{R_a} { | } \forall p \in P_{N_3}:
p \in solved(e)\}} a_{count}(t) * 
\sum_t^{\{e \in PC_{R_b} { | } \forall p \in P_{N_3}:
p \in solved(e)\}} a_{count}(t)$$.

  ** denn eigentlich muss folgendes herauskommen {p1} statt {p1,p2} **

$$ C_{N_3} = \sum_t^{\{e \in PC_{R_a} { | } \forall p \in \{ p_1 \}: 
p \in solved(e)\}} a_{count}(t) * 
\sum_t^{\{e \in PC_{R_b} { | } \forall p \in \{ p_2 \}: 
p \in solved(e)\}} a_{count}(t)$$
$$ C_{N_3} = \sum_t^{\{ (5,3127), (1,19797) \}} a_{count}(t) * 
\sum_t^{\{ (2,21470) \}} a_{count}(t) $$
$$ C_{N_3} = ( 3127 + 19797 ) * 21470 $$
$$ C_{N_3} = 22924 * 21470 $$
$$ C_{N_3} = 492178280 $$


5.2.6.5 node $N_6$ (type 4)

	dto

5.2.6.5 node $N_7$ (type 4)

	dto


5.2.7 What means transformed in case of the result set?

  ** der Abschnitt muss woanders hin **

  To explain let us look at the native??? results of the 
predcounts execution plans.

----
66 ?- secondo('query plz  feed  {b} predcounts [ P2: (.PLZ_b < 50000) ] 
 consume;', Z).
0.init finished
Total runtime ...   Times (elapsed / cpu): 0.800012sec / 0.61sec = 1.3115

410 Z = 
420 [
430   [rel, [tuple, [[Atom, int], [Count, int]]]], 
440   [
441     [1, 21470], 
442     [0, 19797]
443   ]
450 ]

Yes
67 ?- secondo(, A).
0.init finished
Total runtime ...   Times (elapsed / cpu): 0.940014sec / 0.89sec = 1.0562

510 A = 
520 [
530   [rel, [tuple, [[Atom, int], [Count, int]]]], 
540   [
541     [3, 3127], 
542     [2, 19797], 
543     [1, 18343], 
544     [0, 0]
545   ]
550 ]

Yes
----

  As you can see??? the tuples of the result are identified by
0 and 1. The tuple identified by 0 shows the number of rows of the
relation plz doesn't solve??? the sql predicate " PLZ < 50000 " . The
other tuple shows the number of rows solve the sql predicate. But
where should these results are written at the POG?

  For each predcounts execution plan there was created a 
transformation list (lines 231 and 241). For the example we have to
look at the first one. The list contains just one element. So the only
one predicate of predcounts operator corresponds with this element.
This means 21470 rows of plz solve the predicate identified with 2 in
the POG and 19797 rows don't do so???. So one is able to set the
cardinalty of node 2 (binary: 010) to 21470. The value 19797 cann't
be used directly. To used this value one have to combine them with 
the result tuples of the other predcounts queries identified with
atom = 0.

  Let's have a look at the second predounts query (lines 242),
their transformation list (line 241) and their native result list (lines
540-545). The predcounts query builds statistics of two predicates P4
and P1. Using the transformation list one is able to determine the
identifiers of the predicates within the POG. Within the POG the
predicate P1 is identified by 1 (binary 001) and the predicate P4
by 4 (binary 100). Within the predcounts result set the predicate P4
is identified by 2 (binary 010) and P1 by 1 (binary 001). To transform
the identifiers (column atom) of the native result set the identifiers
have to be changed by the following way:

  1 Each identifier of the tuples (lines 541-544) is composed of
the predicate identifiers. By looking at the identifiers in binary mode
you are able to recognize this. The identifier is the sum??? of all
of the identifiers of solved predicates. In case of the example it means:

    * atom=3 - binary 11 - predicates 1 and 2 are solved

    * atom=2 - binary 10 - predicate 2 is solved AND predicate 1 is not

    * atom=1 - binary 01 - predicate 1 is solved AND predicate 2 is not

    * atom=0 - binary 00 - predicates 1 and 2 are not solved

  2 The transformation list (line 241) means:

    * predicate 1 is identified by 1 within the POG

    * predicate 2 is identified by 4 within the POG

  3 In case of predicate 1 there is nothing to do.

  4 In case of predicate 2 it's identifier has to be changed to 4.

  5 As you've seen above in the case of a solved predicate 2 the bit
2\^2 is set. (atom=3 and atom=2) To transform the tuples for using
for POG the bit 2i\^3 has to be set instead of bit 2i\^2. For the
result 11 has to be transformed to 101 and 10 to 100.



  ** hier weiter **


----
66 ?-
----

  
*/
/*
1 Helper predicates

  The following predicates are used for easier programming and
understanding of the main code.

1.1 debug\_write

----    debug_writeln(Message)
----

  The predicate debug\_write writes the given message to 
the screen
without a trailing newline. The message is only written
if the fact debug\_level(X) with X greater then 0 is set.

*/

debug_out(_):- % if X=0 then no debugging messages  
   % if fact debug_level is not set retract fails
   retract((debug_level(X))),
   % reassert the fact
   assert((debug_level(X))),
   % X=0 no debug output requested
   X=0,
   !.
% write debugging messages without concated newline
debug_out(Message):-
   % if fact debug_level is not set retract fails
   retract((debug_level(X))),
   % reassert the fact
   assert((debug_level(X))),
   % X>0 debug output requested
   X > 0,
   % write debug message
   write(Message),
   !.
debug_out(_).       % in any other case ignore call of debug_write

/*
1.2 debug\_outln

----    debug_outln(message)
        debug_outln(Msg1, Msg2)
        debug_outln(Msg1, Msg2, Msg3)
        debug_outln(Msg1, Msg2, Msg3, Msg4)
        debug_outln(Msg1, Msg2, Msg3, Msg4, Msg5)
        debug_outln(Msg1, Msg2, Msg3, Msg4, Msg5, Msg6)
----

  The predicates debug\_outln write the given message(s) to the 
screen. The
given message(s) are concatinated without any delimiter but a newline
will be appended behind??? the last message. The message(s) are only 
written if the fact debug\_level(X) with X greater then 0 is set.

*/

% if X=0 then no debugging messages
debug_outln(_):-
   % if fact debug_level is not set retract fails
   retract((debug_level(X))),
   % reassert the fact
   assert((debug_level(X))),
   % X=0 no debug output requested
   X=0,
   !.
% write debugging messages concated newline
debug_outln(Message):-
   retract((debug_level(X))),
   assert((debug_level(X))),
   X > 0,
   debug_out(Message),
   writeln(''),
   !.
debug_outln(_).

check_debug_level(LevelPred):-
   % if fact debug_level is not set retract fails
   retract((debug_level(LevelSet))),
   % reassert the fact
   assert((debug_level(LevelSet))),
   % check level
   LevelPred =< LevelSet,
   !.

check_debug_list(Pred):-
   % if fact debug_level is not set retract fails
   retract((debug_list(DebugPredList))),
   % reassert the fact
   assert((debug_list(DebugPredList))),
   % check set predicates
   (
   	member(all, DebugPredList),
	not(member(-Pred, DebugPredList));
	member(+Pred, DebugPredList)
   ),
   !.

debug_writeln(Pred, Level, MessageList):-
	% check whether the message has to be written or not
	(
		check_debug_level(Level);
		check_debug_list(Pred)
	),
	write_list(['DEBUG(', Level, '): ', Pred, ': ']),
	write_list(MessageList),
	nl,
	!.

debug_writeln(Pred, Level, _):-
	not(
		check_debug_level(Level);
		check_debug_list(Pred)
	).

%debug_writeln(_, _, _).

write_list([]).
write_list([ nl | Rest ]):-
	nl,
	write_list(Rest).
%write_list([ SubList | Rest ]):-
%	SubList = [ _ | _ ],
%	writeln('['),
%	write_sub_list('   ', SubList),
%	write(']'),
%	write_list(Rest).
write_list([ Atom | Rest ]):-
	write(Atom),
	write_list(Rest).
write_list(Atom):-
	write(Atom).
	
write_sub_list(Head, [ Atom | [] ]):-
	writeln(Head, Atom).
write_sub_list(Head, [ Atom | Rest ]):-
	writeln(Head, Atom),
	write_sub_list(Head, Rest).


/* 
  The following three predicates write two up to four messages 
using the predicates debug\_write/1 and debug\_writeln/1

*/

% write two messages
debug_outln(Msg1,Msg2):-
   debug_out(Msg1),
   debug_outln(Msg2).

%write three messages
debug_outln(Msg1,Msg2,Msg3):-
   debug_out(Msg1),
   debug_out(Msg2),
   debug_outln(Msg3).

% write four messages
debug_outln(Msg1,Msg2,Msg3,Msg4):-
   debug_out(Msg1),
   debug_out(Msg2),
   debug_out(Msg3),
   debug_outln(Msg4).

% write five messages
debug_outln(Msg1,Msg2,Msg3,Msg4,Msg5):-
   debug_out(Msg1),
   debug_out(Msg2),
   debug_out(Msg3),
   debug_out(Msg4),
   debug_outln(Msg5).

% write six messages
debug_outln(Msg1,Msg2,Msg3,Msg4,Msg5,Msg6):-
   debug_out(Msg1),
   debug_out(Msg2),
   debug_out(Msg3),
   debug_out(Msg4),
   debug_out(Msg5),
   debug_outln(Msg6).

/*
1.3 atom\_concat/6

----    atom_concat(Txt1, Txt2, Txt3, Txt4, Txt5, Rslt)
----

  The predicate atom\_concat/6 concatinates five strings to one. It uses
the already defined predicate atom\_concat/3.

*/

% concat more than two strings
atom_concat(Txt1, Txt2, Txt3, Rslt):-
   atom_concat(Txt1, Txt2, TMP1),
   atom_concat(TMP1, Txt3, Rslt).

atom_concat(Txt1, Txt2, Txt3, Txt4, Rslt):-
   atom_concat(Txt1, Txt2, TMP1),
   atom_concat(TMP1, Txt3, TMP2),
   atom_concat(TMP2, Txt4, Rslt).

atom_concat(Txt1, Txt2, Txt3, Txt4, Txt5, Rslt):-
   atom_concat(Txt1, Txt2, TMP1),
   atom_concat(TMP1, Txt3, TMP2),
   atom_concat(TMP2, Txt4, TMP3),
   atom_concat(TMP3, Txt5, Rslt).

/*
1.4 writeln/2 - writeln/6

----    writeln(Txt1, Txt2, Txt3, Txt4, Txt5, Rslt)
----

	* to document *
*/

% concat more than two strings
writeln(Msg1, Msg2):-
   write(Msg1), writeln(Msg2).
writeln(Msg1, Msg2, Msg3):-
   write(Msg1), writeln(Msg2, Msg3).
writeln(Msg1, Msg2, Msg3, Msg4):-
   write(Msg1), writeln(Msg2, Msg3, Msg4).
writeln(Msg1, Msg2, Msg3, Msg4):-
   write(Msg1), writeln(Msg2, Msg3, Msg4).
writeln(Msg1, Msg2, Msg3, Msg4, Msg5):-
   write(Msg1), writeln(Msg2, Msg3, Msg4, Msg5).
writeln(Msg1, Msg2, Msg3, Msg4, Msg5, Msg6):-
   write(Msg1), writeln(Msg2, Msg3, Msg4, Msg5, Msg6).



/*

7 several code

*/














% VJO new part
addCorrelationSizes:-
	build_list_of_predicates,
	listOfRelations(ListOfRelations),
        debug_writeln(addCorrelationSizes/0, 9, ['list of relations built ', ListOfRelations]),
	listOfPredicates(ListOfPredicates),
        debug_writeln(addCorrelationSizes/0, 9, ['list of predicates built ', ListOfPredicates]),
	!,
	% for testing purposes
	%writeln('add relation set of 3 relations'),
	%retract((listOfRelations(R))),
	%append(R,[[rel(ten, a), rel(ten, *), rel(ten, b)]],Rnew),
	%assert((listOfRelations(Rnew))),
	!,
	build_queries(Queries),
	debug_writeln(addCorrelationSizes/0, 9, ['predcount queries built ', Queries]),
	!,
	calculate_predcounts(Queries, PCResults),
	debug_writeln(addCorrelationSizes/0, 9, ['predcount results calculated ', PCResults]),
	!,
	correct_nodeids(PCResults, PCResultsNew),
	debug_writeln(addCorrelationSizes/0, 9, ['node ids corrected ', PCResultsNew]),
	!,
	retractall((corrCummSel(_, _))),
	!,
	createEdgeSelectivities(PCResultsNew),
	debug_writeln(addCorrelationSizes/0, 9, ['edgeSelectivities/3 and resultSize/2 of POG updated']),
	!.

% dann kommt correlations feasible

createEdgeSelectivities([]):-
	removePCResults,
	debug_writeln(createEdgeSelectivities/1, 32, ['end of recursion']).
createEdgeSelectivities([ TupleList | RList ]):-
	debug_writeln(createEdgeSelectivities/1, 39, ['evaluate predcounts results ', TupleList]),
	% predcount Tupel in Fakten storedPCTuple/2 mit korrigierten Count-Werten umsetzen
	removePCResults,
	debug_writeln(createEdgeSelectivities/1, 39, ['predcount results facts removed']),
	storePCResults(TupleList, TupleList),
	debug_writeln(createEdgeSelectivities/1, 39, ['predcount results ', TupleList, ' stored as facts']),
	!,
	% aus PCResults fuer Option entropy die kummulierten Sels berrechnen und als corrCummSel/2 speichern
	storeCummSel,
	debug_writeln(createEdgeSelectivities/1, 39, ['edgeSelectivities based on predcounts results calculated']),
	!,
	% aus storedPCTuple/2 die Fakten edgeSelectivity/3 und resultSize/2 berechnen (?nur ohne Option entropy?)
	storeCorrelationSizes(TupleList, TupleList), % resSize, edgeSel nur ohne entropy
	debug_writeln(createEdgeSelectivities/1, 31, ['facts edgeSelectivity/3 and resultSize/2 of POG updated']),
	!,
	%createEdgeSelectivities2(TupleList, TupleList), % resSize, edgeSel nur ohne entropy
	% Ergebnis der naechsten query verarbeiten
	createEdgeSelectivities(RList).

storeCorrelationSizes([], _):-
	debug_writeln(storeCorrelationSizes/2, 42, ['end of recursion']).
storeCorrelationSizes([ [ NodeID , _ ] | RList ], TupleList):-
	% reale Kardinalitaet des ResultSets der Node ermitteln
	storedPCTuple(NodeID, CorrectCard),
	% resultSize/2 eintragen oder korrigieren
	resetNodeSize(NodeID, CorrectCard),
	debug_writeln(storeCorrelationSizes/2, 41, ['update cardinality of node ', NodeID, ' to ', CorrectCard]),
	% edgeSelectivity/3 fuer Kanten zur Node NodeID speichern oder korrigieren
	storeCorrEdgeSelectivity(NodeID),
	debug_writeln(storeCorrelationSizes/2, 41, ['update edgeSelectivity/3 of edges to node ', NodeID]),
	%createEdgeSelectivities3(NodeID, CorrectCard, NodeID),
	storeCorrelationSizes(RList, TupleList).
% unexpected call
storeCorrelationSizes(A, B):-
	error_exit(storeCorrelationSizes/2, ['unexpected call',[A, B]]).

% zwei Fakten zum besseren Lesen
storeCorrEdgeSelectivity(0):- % nichts tun
	debug_writeln(storeCorrEdgeSelectivity/1, 52, ['do nothing for node 0']).
storeCorrEdgeSelectivity(0.0):- % nichts tun
	debug_writeln(storeCorrEdgeSelectivity/1, 52, ['do nothing for node 0.0']).
storeCorrEdgeSelectivity(NodeID):-
	findall(PrevNodeID, edge(PrevNodeID, NodeID, _, _, _, _), PrevNodeIDs),
	debug_writeln(storeCorrEdgeSelectivity/1, 59, ['previous nodes of node ', NodeID, ' are ', PrevNodeIDs]),
	storeCorrEdgeSelectivity2(NodeID, PrevNodeIDs),
	debug_writeln(storeCorrEdgeSelectivity/1, 51, ['edgeSelectivity/3 of all edges updated/inserted to node ', NodeID, ' from ', PrevNodeIDs]).

storeCorrEdgeSelectivity2(_, []):-
	debug_writeln(storeCorrEdgeSelectivity2/2, 62, ['end of recursion']).
storeCorrEdgeSelectivity2(NodeID, [ PrevNodeID | PrevNodeIDs ]):-
	storedPCTuple(NodeID, NodeCard),
	storeCorrEdgeSelectivity3(PrevNodeID, NodeID, NodeCard),
	debug_writeln(storeCorrEdgeSelectivity2/2, 61, ['edgeSelectivity/3 of edge (', PrevNodeID,',',NodeID,') upserted']),
	storeCorrEdgeSelectivity2(NodeID, PrevNodeIDs).

resetNodeSize(0, _):- % is still the correct size
	debug_writeln(resetNodeSize/2, 52, ['node size of node 0 is still correct']).
resetNodeSize(0.0, _):- % is still the correct size
	debug_writeln(resetNodeSize/2, 52, ['node size of node 0.0 is still correct']).
resetNodeSize(NodeID, Size) :- 
	resultSize(Node,_),
	Node=:=NodeID, % wegen int und real-Mischung!
	retract((resultSize(Node,_))),
	setNodeSize(Node, Size),
	debug_writeln(resetNodeSize/2, 51, ['size of node ', Node, ' updated to ', Size]).
resetNodeSize(NodeID, Size) :- 
	Node is NodeID + 0.0, % NodeID has to be real
	setNodeSize(Node, Size),
	debug_writeln(resetNodeSize/2, 53, ['size ', Size, ' of node ', Node, ' inserted']).

storeCorrEdgeSelectivity3(PrevNodeID, NodeID, NodeCard):-
	storedPCTuple(PrevNodeID, PrevNodeCard), % Card ist bereits korrigiert
	storeCorrEdgeSelectivity4(PrevNodeID, NodeID, PrevNodeCard, NodeCard),
	debug_writeln(storeCorrEdgeSelectivity3/3, 71, ['edgeSelectivity/3 of edge (', [PrevNodeID,NodeID], ') using ', [PrevNodeCard, NodeCard]]).
storeCorrEdgeSelectivity3(PrevNodeID, NodeID, _):-
	debug_writeln(storeCorrEdgeSelectivity3/3, 72, ['ignore edge ',[PrevNodeID,NodeID]]).

removeEdgeSelectivity(0, NodeID):-
	NodeIDreal is NodeID + 0.0,
	%retractall((corrEdgeSelectivity(0, NodeIDreal, _))),
	retractall((edgeSelectivity(0, NodeIDreal, _))),
	debug_writeln(removeEdgeSelectivity/2, 91, ['edgeSelectivity/3 of edge (', [0, NodeIDreal], ') removed']).
removeEdgeSelectivity(PrevNodeID, NodeID):-
	% for real nodes
	PrevNodeIDreal is PrevNodeID + 0.0,
	NodeIDreal is NodeID + 0.0,
	%retractall((corrEdgeSelectivity(PrevNodeIDreal, NodeIDreal, _))),
	retractall((edgeSelectivity(PrevNodeIDreal, NodeIDreal, _))),
	debug_writeln(removeEdgeSelectivity/2, 92, ['edgeSelectivity/3 of edge (', [PrevNodeIDreal, NodeIDreal], ') removed']).



storeCorrEdgeSelectivity4(PrevNodeID, NodeID, 0, _):-
	removeEdgeSelectivity(PrevNodeID, NodeID),
	storeCorrEdgeSelectivity5(PrevNodeID, NodeID, 1),
	debug_writeln(storeCorrEdgeSelectivity4/4, 82, ['edgeSelectivity(', [PrevNodeID, NodeID, 1], ') stored']).

storeCorrEdgeSelectivity4(PrevNodeID, NodeID, _, 0):-
	removeEdgeSelectivity(PrevNodeID, NodeID),
	storeCorrEdgeSelectivity5(PrevNodeID, NodeID, 0),
	debug_writeln(storeCorrEdgeSelectivity4/4, 83, ['edgeSelectivity(', [PrevNodeID, NodeID, 0], ') stored']).

storeCorrEdgeSelectivity4(PrevNodeID, NodeID, PrevNodeCard, NodeCard):-
	removeEdgeSelectivity(PrevNodeID, NodeID),
	EdgeSel is NodeCard / PrevNodeCard,
	storeCorrEdgeSelectivity5(PrevNodeID, NodeID, EdgeSel),
	debug_writeln(storeCorrEdgeSelectivity4/4, 81, ['edgeSelectivity(', [PrevNodeID, NodeID, EdgeSel], ') stored']).

storeCorrEdgeSelectivity5(0, NodeID, EdgeSel):-
	% for real nodes
	NodeIDreal is NodeID + 0.0,
	assert((edgeSelectivity(0, NodeIDreal, EdgeSel))),
	debug_writeln(storeCorrEdgeSelectivity5/3, 92, ['edgeSelectivity(', [0, NodeIDreal, EdgeSel], ') asserted']).
	
storeCorrEdgeSelectivity5(PrevNodeID, NodeID, EdgeSel):-
	% for real nodes
	PrevNodeIDreal is PrevNodeID + 0.0,
	NodeIDreal is NodeID + 0.0,
	assert((edgeSelectivity(PrevNodeIDreal, NodeIDreal, EdgeSel))),
	debug_writeln(storeCorrEdgeSelectivity5/3, 91, ['edgeSelectivity(', [PrevNodeIDreal, NodeIDreal, EdgeSel], ') asserted']).
	
storeCummSel:-
	storedPCTuple(0, BaseCard),
	% fuer alle Nodes die Kummulierte Sel speichern
	findall([NodeID, NodeCard], storedPCTuple(NodeID, NodeCard), NodeCards),
	storeCummSel2(BaseCard, NodeCards),
	debug_writeln(storeCummSel/0, 41, ['joint selectivity of all nodes tuples ', NodeCards, ' stored']).

storeCummSel2(_, []):-
	debug_writeln(storeCummSel2/2, 52, ['end of recursion']).
storeCummSel2(BaseCard, [ [ NodeID, NodeCard ] | NodeCards ]):-
	storeCummSel3(NodeID, NodeCard, BaseCard),
	debug_writeln(storeCummSel2/2, 51, ['joint selectivity of node ', NodeID, ' calculated using cardinalities ', [NodeCard, BaseCard]]),
	storeCummSel2(BaseCard, NodeCards).

storeCummSel3(NodeID, _, 0.0):-
	storeCummSel3(NodeID, _, 0),
	debug_writeln(storeCummSel3/3, 52, ['use storeCummSel3(', [NodeID, unset, 0], ')']).
storeCummSel3(NodeID, _, 0):-
	retractall((corrCummSel(NodeID, _))),
	assert((corrCummSel(NodeID, 1))),
	debug_writeln(storeCummSel3/3, 53, ['cardinality of base node is 0 - corrCummSel(', [NodeID, 1],') asserted']).
storeCummSel3(NodeID, NodeCard, BaseCard):-
	CummSel is NodeCard / BaseCard,
	storeCummSel(NodeID, CummSel),
	debug_writeln(storeCummSel3/3, 52, ['corrCummSel(', [NodeID, 1], ') asserted']).

storeCummSel(NodeID, CummSel):-
	retractall((corrCummSel(NodeID, _))),
	assert((corrCummSel(NodeID, CummSel))).

removePCResults:-
	retractall((storedPCTuple(_, _))),
	debug_writeln(removePCResults/0, 41, ['facts storedPCTuple/2 removed']).

storePCResults([], _):-
	debug_writeln(storePCResults/2, 42, ['end of recursion']).
storePCResults([ [ NodeID, _ ] | RList ], PCList):-
	correctNodeCard(NodeID, PCList, CorrectCard),
	!,
	assert((storedPCTuple(NodeID, CorrectCard))),
	debug_writeln(storePCResults/2, 41,['corrected node cardinality ', CorrectCard, ' asserted for node ', NodeID, ' as fact storedPCTuple/2']),
	!,
	storePCResults(RList, PCList).

correctNodeCard(_, [], 0):-
	debug_writeln(correctNodeCard/3, 52, ['end of recursion']).
correctNodeCard(NodeID, [ [ NID , NC ] | RList ], NewCard):-
	correctNodeCard(NodeID, RList, OldCard),
	getAdjustment(NodeID, NID, NC, Adjustment),
	NewCard is OldCard + Adjustment,
	debug_writeln(correctNodeCard/3, 51, ['cardinality of node ', NodeID, ' adjusted by ', Adjustment]).

getAdjustment(NodeID, NID, NC, Adjustment):-
	equals_and(NodeID, NID, NodeID),
	Adjustment = NC,
	debug_writeln(getAdjustment/4, 61, ['adjustment ', Adjustment, ' of node ', NID, ' for node ', NodeID]).
getAdjustment(NodeID, NID, _, 0):-
	debug_writeln(getAdjustment/4, 61, ['no adjustment of node ', NID, ' for node ', NodeID]).

	


/*
  Neuimplemeniterung feasible

*/
	
corrFeasible(MP, JP, MP2, JP2):-
	% corrCummSel/2 existiert bereits
	optimizerOption(correlations),
% corrCummSel/2
% => Entropy zu CorrCummSel-Fakten
%
	storeMarginalSels(MP), % MP von assignEntropyCost
	storeEntropyEdgeSelectivities(JP), % all edges stored as corrEdgeSelectivity/3
	%storeCorrEdgeSelectivities, % all edges stored as corrEdgeSelectivity/3
	%storeJointSels(JP), % JP von assignEntropyCost
	!,
	corrFeasible,
	!,
	renormAllCummSels,
	!,
	loadMarginalSels(MP2),
	findall([NodeID, NodeSel], corrCummSel(NodeID, NodeSel), JP2),
	!,
	writeln('Corr MP2: ', MP2),
	writeln('Corr JP2: ',JP2).

renormAllCummSels:-
	highNode(HN),
	corrCummSel(NodeID, BaseSel),
	NodeID =:= 0,
	renormAllCummSels2(BaseSel, HN).

renormAllCummSels2(1, _).
renormAllCummSels2(_, NodeID):-
	NodeID < 0.
renormAllCummSels2(BaseSel, NodeID):-
	renormAllCummSel(BaseSel, NodeID),
	NodeIDNew is NodeID - 1,
	renormAllCummSels2(BaseSel, NodeIDNew).

renormAllCummSel(BaseSel, NodeID):-
	corrCummSel(NodeID2, CummSel),
	NodeID2 =:= NodeID,
	retract((corrCummSel(NodeID2, CummSel))),
	CummSelNew is CummSel / BaseSel,
	assert((corrCummSel(NodeID2, CummSelNew))),
	debug_outln('renormAllCummSel: ',[NodeID2, CummSel, CummSelNew]).

loadMarginalSels(CorrectedMarginalPreds):-
	findall(PredID, corrMarginalSel(PredID, _), MarginalPredIDs),
	loadMarginalSels2(MarginalPredIDs, CorrectedMarginalPreds).

loadMarginalSels2([], []).
loadMarginalSels2([ PredID | PredIDs ], [ [ PredID, CummSel ] | MP ]):-
	corrCummSel(PredID, CummSel),
	!,
	loadMarginalSels2(PredIDs, MP).
loadMarginalSels2([ PredID | PredIDs ], [ [ PredID, MargSel ] | MP ]):-
	corrMarginalSel(PredID, MargSel),
	!,
	loadMarginalSels2(PredIDs, MP).
% unexpected call
loadMarginalSels2(A, B):-
	error_exit(loadMarginalSels2/2,'unexpected call',[A,B]).
	
storeMarginalSels(MP):-
	retractall((corrMarginalSel(_, _))),
	storeMarginalSels2(MP).

storeMarginalSels2([]).
storeMarginalSels2([ [ PredID, MargSel ] | MP ]):-
	resetMarginalSel(PredID, MargSel),
	storeMarginalSels2(MP).

resetMarginalSel(PredID, MargSel):-
	retractall((corrMarginalSel(PredID, _))),
	assert((corrMarginalSel(PredID, MargSel))).


% VJO Eintrittspunkt
corrFeasible:-
	findall([A,B],corrCummSel(A,B),X),writeln('corrCummSel/2: ',X),
	highNode(HN),
	corrFeasible2(HN).

corrFeasible2(0).
corrFeasible2(0.0).
corrFeasible2(NodeID):-
	% Kanten mit Selektivitaet zu 0 modifizieren
	correct0Selectivity(NodeID),
	!,
	% Kanten mit Selektivitaet 1 modifizieren
	% falls Betrachtung indirekter Vorgaenger noetig wird,
	% erfolgt dies durch indirekte Rekursion in correct1Selectivity/2
	findall(PrevNodeID, edge(PrevNodeID, NodeID, _, _, _, _), PrevNodeIDs),
	debug_outln('corrFeasible2: check previous nodes ',[ NodeID, PrevNodeIDs]),
	corrFeasible3(NodeID, PrevNodeIDs),
	NextNodeID is NodeID - 1,
	!,
	corrFeasible2(NextNodeID).

corrFeasible3(_, []).
corrFeasible3(NodeID, [ PrevNodeID | Rest ]):-
	correct1Selectivity(PrevNodeID,NodeID),
	corrFeasible3(NodeID, Rest).

correct0Selectivity(NodeID):-
	corrCummSel(NodeID, CummSel),
	CummSel > 0,
	debug_outln('correct0Selectivity(1): keep ', [NodeID, CummSel]).
correct0Selectivity(NodeID):-
	corrCummSel(NodeID, _),
	debug_outln('correct0Selectivity(2): to increase ',NodeID),
	% jede VorgaengerNode hat kleinere ID!
	increaseSelectivityRecursive(NodeID, NodeID).
% dann ist nichts zu korrigieren
correct0Selectivity(NodeID):-
	not(corrCummSel(NodeID, _)).
correct0Selectivity(NodeID):-
	error_exit(correct0Selectivity/1, 'unexpected call',[NodeID]).

correct1Selectivity(PrevNodeID,NodeID):-
	corrCummSel(NodeID, CummSel),
	corrCummSel(PrevNodeID, PrevCummSel),
	% dann ist die Selektivitaet der Kante kleiner als 1
	CummSel < PrevCummSel,
	debug_outln('correct1Selectivity(1): keep ', [PrevNodeID,NodeID,CummSel,PrevCummSel]).
correct1Selectivity(PrevNodeID,NodeID):-
	corrCummSel(NodeID, CummSel),
	corrCummSel(PrevNodeID, CummSel),
	% 10^-19 = Delta is CummSel - PrevCummSel,
	%writeln('correct1Selectivity(2): to increase check delta ',[CummSel,PrevCummSel,Delta]),
	% Kante mit Selektivitaet 0 gefunden
	debug_outln('correct1Selectivity(2): to increase ', [PrevNodeID,NodeID,CummSel]),
	% alle Vorgaengerknoten erhoehen
	increaseSelectivityRecursive(PrevNodeID,PrevNodeID).
correct1Selectivity(PrevNodeID,NodeID):-
	corrCummSel(NodeID, CummSel),
	corrCummSel(PrevNodeID, PrevCummSel),
	% Kante mit Selektivitaet >1 gefunden
	CummSel > PrevCummSel,
	% das kann nicht sein => Fehler in Datenbasis
	%findall([A,B],corrCummSel(A,B),X),writeln('corrCummSel/2: ',X),
	writeln('WARN: correct1Selectivity: Inkonsistenz gefunden [ vonNode, zuNode ]',[PrevNodeID,NodeID],
		' kummulierte Selektivitaeten [vonSel, nachSel] ',[PrevCummSel,CummSel],
		' - wird korrigiert'),
	% Delta berrechnen
	Delta is CummSel - PrevCummSel,
	% alle Vorgaengerknoten erhoehen (Inkonsistenz korrigieren)
	increaseSelectivityRecursive(PrevNodeID,PrevNodeID, Delta),
	% Kante mit Selektivitaet 1 entfernen
	increaseSelectivityRecursive(PrevNodeID,PrevNodeID).
% wenn fuer den unmittelbaren Vorgaenger des Knotens keine Selektivitaet bekannt ist,
% dann pruefen, ob sie fuer indirekte Vorgaegner bekannt sind
% (Suche nur bis bekannten gefunden)
correct1Selectivity(PrevNodeID,NodeID):-
	corrCummSel(NodeID, _),
	not(corrCummSel(PrevNodeID, _)),
	% unmittelbarer Vorgaenger nicht bekannt,
	% dann fuer all dessen Vorgaenger pruefen
	findall(NewPrevNodeID, edge(NewPrevNodeID, PrevNodeID, _, _, _, _), NewPrevNodeIDs),
	debug_outln('previous nodes (recursive search): ',[ NodeID, PrevNodeID, NewPrevNodeIDs]),
	corrFeasible3(NodeID, NewPrevNodeIDs).
% es werden alle N´Knoten geprueft,
% falls ein Knoten nicht bekannt, dann ignorieren
correct1Selectivity(_,NodeID):-
	not(corrCummSel(NodeID, _)).
correct1Selectivity(PrevNodeID,NodeID):-
	error_exit(correct1Selectivity/2,'unexpected call',[PrevNodeID,NodeID]).

getEpsilon(Epsilon):-
	retract(corrEpsilon(Epsilon)),
	assert(corrEpsilon(Epsilon)),
	!.
getEpsilon(0.001):-
	!.

increaseSelectivityRecursive(NodeID, ChildNodeID):-
	getEpsilon(Epsilon),
	increaseSelectivityRecursive(NodeID, ChildNodeID, Epsilon).

increaseSelectivityRecursive(NodeID, _, _):-
	NodeID < 0.
increaseSelectivityRecursive(NodeID, ChildNodeID, Epsilon):-
	% % Node A ist VG von Node B, falls A=and(A,B)
	equals_and(NodeID, NodeID, ChildNodeID),
	increaseSelectivity(NodeID, Epsilon),
	% jede VorgaengerNode hat kleinere ID!
	NewNodeID is NodeID - 1,
	increaseSelectivityRecursive(NewNodeID, ChildNodeID, Epsilon).
increaseSelectivityRecursive(NodeID, ChildNodeID, Epsilon):-
	% jede VorgaengerNode hat kleinere ID!
	NewNodeID is NodeID - 1,
	increaseSelectivityRecursive(NewNodeID, ChildNodeID, Epsilon).

increaseSelectivity(NodeID, Epsilon):- % VJO int oder real
	retract((corrCummSel(NodeID, CummSel))),
	CummSelNew is CummSel + Epsilon,
	assert((corrCummSel(NodeID, CummSelNew))),
	debug_outln('increaseSelectivity: NodeID increased by Epsilon to value ',[NodeID,Epsilon,CummSelNew]).

storeEntropyEdgeSelectivities([]).
storeEntropyEdgeSelectivities([ [ _, NodeID, CummEdgeSel ] | Rest]):-
	%storeCummEdgeSelectivity(PrevNodeID, NodeID, CummEdgeSel),
	storeCummSel(NodeID, CummEdgeSel),
	storeEntropyEdgeSelectivities(Rest).


correct_nodeids([], []).
correct_nodeids([ PCResult | PCResults ], [ PCResultNew | PCResultsNew ]):-
	correct_nodeids2(PCResult, PCResultNew),
	correct_nodeids(PCResults, PCResultsNew).

correct_nodeids2([], []).
correct_nodeids2([ [ NodeID | Rest ] | PCTuples ], [ [ NodeIDNew | Rest ] | PCTuplesNew ]):-
	correct_nodeid(NodeID, NodeIDNew),
	!,
	correct_nodeids2(PCTuples, PCTuplesNew).

correct_nodeid(NodeID, NodeIDNew):-
	node(NodeIDNew, _, _),
	NodeIDNew =:= NodeID.
correct_nodeid(NodeID, _):-
	writeln('Fehler: kann Fakt node/3 zu Node ',NodeID,' nicht finden').

equals_and(A, B, C):-
	A2 is round(A),
	B2 is round(B),
	C2 is round(C),
	A2 =:= B2 /\ C2.


	%
get_real_nodeid(NodeID, NodeIDReal):-
	node(NodeIDReal, _, _),
	NodeID =:= NodeIDReal,
	!.


find_cheapest_Path([], []).
	%writeln('xxxxx1== ').
	%nicht join preds ignorieren
find_cheapest_Path([ JoinEdge | JoinEdges ], CheapestEdge):-
	JoinEdge = [ TargetNodeID, _, _ ],
	node(TargetNodeID, [Predicate], _),
	% is a join ?
	Predicate = pr(_, _),
	!,
	%writeln('xxxxx2-> ',[TargetNodeID]),
	find_cheapest_Path(JoinEdges, CheapestEdge).
find_cheapest_Path([ JoinEdge | JoinEdges], CheapestEdge):-
	%writeln('xxxxx4-> ',[JoinEdge]),
	find_cheapest_Path(JoinEdges, CheapestEdgeOld),
	%writeln('xxxxx4<- ',[CheapestEdgeOld]),
	!,
	JoinEdge = [ _, JoinEdgeCost, _ ],
	(
		CheapestEdgeOld = [ _, CheapestEdgeOldCost, _ ],
		JoinEdgeCost >= CheapestEdgeOldCost
		->
		CheapestEdge = CheapestEdgeOld;
		CheapestEdge = JoinEdge
	).

and(X,Y):-
	X,
	Y.

contains(E1, [ E2 ]):-
	E1 =:= E2.
contains(E1, [ E2 | _ ]):-
	E1 =:= E2.
contains(E,[ _ | S ]):-
	not(S=[]),
	contains(E,S).




correct_predlist([], _, []).
correct_predlist([ [JoinPred] | PredIdentList ], UsedJoinPredicate, [ JoinPred | PredIdentListNew]):-
	JoinPred =\= UsedJoinPredicate,
	debug_outln('xxx2',[[JoinPred]]),
	!,
	correct_predlist(PredIdentList, UsedJoinPredicate, PredIdentListNew).
correct_predlist([ Pred | PredIdentList ], UsedJoinPredicate, [ Pred | PredIdentListNew]):-
	debug_outln('xxx3',[Pred]),
	correct_predlist(PredIdentList, UsedJoinPredicate, PredIdentListNew).


% alle JoinPreds fuer Relation ermitteln
get_join_predicates(Relation, JoinPredicateIDs, JoinPredicates):-
	listOfPredicates(Predicates),
	get_join_predicates2(Relation, Predicates, JoinPredicateIDs, JoinPredicates).

get_join_predicates2(_, [], [], []).
get_join_predicates2(Relation, [ Predicate | Predicates ], [ [PredID] | JoinPredicateIDs ], [ Predicate | JoinPredicates ]):-
	Predicate = [ PredID, PredRelation, pr(_, _, _) ],
	subset(PredRelation, Relation),
	!,
	get_join_predicates2(Relation, Predicates, JoinPredicateIDs, JoinPredicates).
get_join_predicates2(Relation, [ _ | Predicates ], JoinPredicateIDs, JoinPredicates):-
	get_join_predicates2(Relation, Predicates, JoinPredicateIDs, JoinPredicates).

% alle SelPreds ermitteln
get_sel_predicates(Relation, SelPredicateIDs, SelPredicates):-
	listOfPredicates(Predicates),
	get_sel_predicates2(Relation, Predicates, SelPredicateIDs, SelPredicates).

get_sel_predicates2(_, [], [], []).
get_sel_predicates2(Relation, [ Predicate | Predicates ], [ PredID | SelPredicateIDs ], [ Predicate | SelPredicates ]):-
	Predicate = [ PredID, PredRelation, pr(_, _) ],
	(
		is_list(Relation)
		->
		member(PredRelation, Relation);
		Relation = PredRelation
	),
	!,
	get_sel_predicates2(Relation, Predicates, SelPredicateIDs, SelPredicates).
get_sel_predicates2(Relation, [ _ | Predicates ], SelPredicateIDs, SelPredicates):-
	get_sel_predicates2(Relation, Predicates, SelPredicateIDs, SelPredicates).

%alle Kombis von JoinPreds ermitteln, die reichen alle Relationen zu verbinden
get_all_join_sets(Relation, JoinPredicates, JoinSets):-
	debug_outln('get_all_join_sets: ',[Relation, JoinPredicates, JoinSets]),
	flatten([Relation], RelationList),
	findall(JoinSet, get_join_set0(RelationList, JoinPredicates, [], JoinSet), JoinSets).

get_join_set0(Relation, JoinPredicates, [], JoinSet):-
	debug_outln('get_join_set0: ',[Relation, JoinPredicates, [], JoinSet]),
	get_join_set(Relation, JoinPredicates, [], JoinSet).

get_join_set(Relations, _, JointRelations, []):-
	set_equal(Relations, JointRelations),
	!.
get_join_set(Relations, [ JoinPredicate | JoinPredicates ], JointRelations, [ JoinPredicateID | JoinSet ]):-
	JoinPredicate = [ JoinPredicateID, PredRelations, _ ],
	not(subset(PredRelations, JointRelations)),
	append(PredRelations, JointRelations, JointRelationsTmp),
	list_to_set(JointRelationsTmp, JointRelationsNew),
	get_join_set(Relations, JoinPredicates, JointRelationsNew, JoinSet).
get_join_set(Relations, [ JoinPredicate | JoinPredicates ], JointRelations, JoinSet):-
	JoinPredicate = [ _, _, _ ],
	%ist Alternativpfad fuer findall, deshalb nicht: subset(PredRelations, JointRelations),
	get_join_set(Relations, JoinPredicates, JointRelations, JoinSet).
	
% Kosten aller Kombis ermitteln
% geht von Unabhaengigkeit der Predikate aus
get_smallest_join([ JoinSet | [] ], [JoinSet], Selectivity):-
	get_join_selectivity(JoinSet, Selectivity).
get_smallest_join([ JoinSet | JoinSets ], JoinSetNew, SelectivityNew):-
	get_smallest_join(JoinSets, JoinSetOld, SelectivityOld),
	get_join_selectivity(JoinSet, Selectivity),
	(
		Selectivity < SelectivityOld
		->
		SelectivityNew = Selectivity,
		JoinSetNew = JoinSet;
		SelectivityNew = SelectivityOld,
		JoinSetNew = JoinSetOld
	).

get_join_selectivity([], 1).
get_join_selectivity([ JoinPredID | JoinSet ], SelectivityNew):-
	get_join_selectivity(JoinSet, SelectivityOld),
	get_joinpred_selectivity(JoinPredID, Selectivity),
	SelectivityNew is SelectivityOld * Selectivity.
get_join_selectivity(Unknown, _):-
	writeln('ERROR: unexpected call get_join_selectivity(',Unknown,',_)').

get_joinpred_selectivity(JoinPredID, Selectivity):-
	get_real_nodeid(JoinPredID, JoinPredIDReal),
	edgeSelectivity(0, JoinPredIDReal, Selectivity).
get_joinpred_selectivity(JoinPredID, 1):-
	writeln('WARN: get_joinpred_selectivity/2: no edgeSelectivity/3 found for node ',JoinPredID,' - use selectivity of 1').

% Algo Dijkstra ist besser, aber noch unnoetig
get_cheapest_path(JoinSet, CheapestJoinPath):-
	flatten(JoinSet, JoinSetTmp),
	sum_elements_of_list(JoinSetTmp, TargetNodeID),
	findall([Cost, PredIDs, Path], get_path_cost(0, TargetNodeID, Path, PredIDs, Cost), Paths),
	msort(Paths, OrderedPaths),
	OrderedPaths = [ [ _, _, CheapestJoinPath ] | _ ].

	
get_path_cost(NodeID2, NodeID, [], [], 0):-
	NodeID2 =:= NodeID.
get_path_cost(LastNodeID, TargetNodeID, [ CostEdge | CostEdges ], [ PredID | PredIDs ], CostNew):-
	planEdge(LastNodeID, NewNodeID, PredPath, _),
	PredID is NewNodeID - LastNodeID,
	equals_and(PredID,PredID,TargetNodeID),
	%writeln(' versuche von ', LastNodeID, ' nach ', NewNodeID),
	edgeSelectivity(LastNodeID, NewNodeID, Selectivity),
	cost(PredPath, Selectivity, ResultSize, Cost),
	CostEdge = costEdge(LastNodeID, NewNodeID, PredPath, NewNodeID, ResultSize, Cost),
	get_path_cost(NewNodeID, TargetNodeID, CostEdges, PredIDs, CostOld),
	CostNew is CostOld + Cost.

sum_elements_of_list([], 0).
sum_elements_of_list([ Elem | List ], SumNew):-
	sum_elements_of_list(List, SumOld),
	SumNew is SumOld + Elem.

% VJO Nachrichten
info_continue(Pred, MessageList):-
	write_list([ 'INFO: ', Pred, ': ']),
	write_list(MessageList),
	nl.
warn_continue(Pred, MessageList):-
	write_list([ 'WARN: ', Pred, ': ']),
	write_list(MessageList),
	nl.
error_exit(Pred, MessageList):-
	write_list([ 'ERROR: ', Pred, ': ']),
	write_list(MessageList),
	nl,
	abort.


equals_or_contains(Relation, Relation2):-
	set_equal(Relation, Relation2),
	debug_outln('equals_or_contains: Relation of Predicate equals Relation of query to build').

equals_or_contains([ Relation2 | _ ], Relation2):-
	debug_outln('equals_or_contains: Relation ',Relation2,
		' of Predicate in Relations of query to build').

equals_or_contains([], Relation2):-
	debug_outln('equals_or_contains: Relation ',Relation2,
		' of Predicate not in Relations of query to build'),
	fail.

equals_or_contains([ _ | Relations ], Relation2):-
	equals_or_contains(Relations,Relation2).



set_debug_list(List):-
	retractall((debug_list(_))),
	assert((debug_list(List))).

set_debug_level(Level):-
	retractall((debug_level(_))),
	assert((debug_level(Level))).

