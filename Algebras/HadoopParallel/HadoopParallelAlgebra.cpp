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

//paragraph [1] Title: [{\Large \bf] [}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]
//[newpage] [\newpage]

[1] Implementation of HadoopParallelAlgebra

April 2010 Jiamin Lu

[TOC]

[newpage]

1 Abstract

HadoopParallelAlgebra implements all relevant operators of integrating
Hadoop and Secondo together to execute some parallel operations.
This algebra includes follow operators:

  * ~doubleexport~. Mix two relations into (key, value) style relation.

  * ~parahashjoin~. Execute join operation on a hash partitioned relation
but includes tuples of different schemes.

1 Includes,  Globals

*/

#include <vector>
#include <iostream>
#include <string>
#include "RelationAlgebra.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "Counter.h"
#include "TupleIdentifier.h"
#include "LogMsg.h"
#include "RTreeAlgebra.h"
#include "ListUtils.h"
#include "HadoopParallelAlgebra.h"
#include "FTextAlgebra.h"
#include "Symbols.h"
#include "Base64.h"
#include "regex.h"
#include "FileSystem.h"

using namespace symbols;
using namespace std;

extern NestedList* nl;
extern QueryProcessor* qp;

/*
2 Operator ~doubleexport~

This operator is usually used in the map function of MapReduce model.
The main work of this operator is to mix tuples from two different
relations of different schemes into one relation following
the (key:string, value:text) pair schema.
The operator extracts the operand field values out as the keys,
and the value field contains two elements,
one is the complete original tuple as ~tupleVal~
and the other one is an integer number ~SI~(source indicator: 1 or 2)
that is used to denote which source relation the ~tupleVal~ comes from.
At the same time, we use Base 64 code to represent the tuple value,
not the nestedList style, because invoking the Tuple class's ~Out~
function is very expensive.

Since the result relation follows the (key, value) style,
the MapReduce module can read the tuples inside this relation,
and group the tuples come from different
source relations but with a same key value
together into one reduce function.
Therefore in each reduce function,
we can call Secondo to process some queries
only for these tuples with a same key value.

2.1 Specification of Operator ~doubleexport~

*/

struct doubleExportInfo : OperatorInfo
{
  doubleExportInfo()
  {
    name = "doubleexport";
    signature =
        "stream (tuple((a1 t1) ... (ai ti) ... (an tm)))"
        "x stream (tuple((b1 p1) ... (bj tj) ... (bm tm)))"
        "x ai x bj -> "
        "stream (tuple (key:string) (value:string))";
    syntax = "_ _ doubleexport[_ , _]";
    meaning = "Mix two relations into (key, value) pairs";
  }
};

/*
2.1 Type Mapping of Operator ~doubleexport~

---- ((stream (tuple((a1 t1) ... (ai string) ... (an tm))))
     (stream (tuple((b1 p1) ... (bj string) ... (bm pm)))) ai bj )
     -> ((stream (tuple (key: string) (value: text)))
     APPEND (i j))
----

*/
ListExpr doubleExportTypeMap(ListExpr args)
{
  if (nl->ListLength(args) != 4)
  {
    ErrorReporter::ReportError(
      "Operator doubleexport expect a list of four arguments");
    return nl->TypeError();
  }

  if (listutils::isTupleStream(nl->First(args))
      && listutils::isTupleStream(nl->Second(args))
      && listutils::isSymbol(nl->Third(args))
      && listutils::isSymbol(nl->Fourth(args)))
  {
    //Get the indices of two indicated key attributes
    ListExpr tupTypeA, tupTypeB;
    tupTypeA = nl->Second(nl->First(args));
    tupTypeB = nl->Second(nl->Second(args));

    ListExpr attrTypeA, attrTypeB;
    ListExpr tupListA = nl->Second(tupTypeA);
    string attrAName = nl->SymbolValue(nl->Third(args));
    int attrAIndex =
        listutils::findAttribute(tupListA,attrAName,attrTypeA);
    if (attrAIndex <= 0)
    {
      ErrorReporter::ReportError(
        "Attributename " + attrAName
        + " not found in the first argument");
      return nl->TypeError();
    }

    ListExpr tupListB = nl->Second(nl->Second(nl->Second(args)));
    string attrBName = nl->SymbolValue(nl->Fourth(args));
    int attrBIndex =
        listutils::findAttribute(tupListB,attrBName,attrTypeB);
    if (attrBIndex <= 0)
    {
      ErrorReporter::ReportError(
        "Attributename " + attrBName
        + " not found in the second argument");
      return nl->TypeError();
    }

    if (listutils::isDATA(attrTypeA)
      && listutils::isDATA(attrTypeB)
      && nl->Equal(attrTypeA, attrTypeB))
    {
      ListExpr attrList = nl->TwoElemList(
          nl->TwoElemList(nl->StringAtom("keyT",false),
              nl->SymbolAtom(STRING)),
          nl->TwoElemList(nl->StringAtom("valueT",false),
              nl->SymbolAtom(TEXT)));
      NList AttrList(attrList, nl);
      NList tupleStreamList =
          NList(NList().tupleStreamOf(AttrList));

      return nl->ThreeElemList(
                 nl->SymbolAtom("APPEND"),
                 nl->TwoElemList(nl->IntAtom(attrAIndex),
                                 nl->IntAtom(attrBIndex)),
                 tupleStreamList.listExpr());
    }
    else
    {
      ErrorReporter::ReportError(
        "Operator doubleexport expect "
          "two same and DATA kind key types.");
      return nl->TypeError();
    }
  }
  else
  {
    ErrorReporter::ReportError(
      "Operator doubleexport expect: "
      "stream (tuple((a1 t1) ... (ai ti) ... (an tm)))"
      "x stream (tuple((b1 p1) ... (bj tj) ... (bm tm)))"
      "x ai x bj -> stream (tuple (key:text) (value:text))");
    return nl->TypeError();
  }
}

/*
2.2 Value Mapping of Operator ~doubleexport~

*/

int doubleExportValueMap(Word* args, Word& result,
    int message, Word& local, Supplier s)
{
  deLocalInfo *localInfo;

  switch (message)
  {
  case OPEN:
    qp->Open(args[0].addr);
    qp->Open(args[1].addr);

    localInfo =
        new deLocalInfo(args[0], args[4], args[1], args[5], s);
    local = SetWord(localInfo);
    return 0;
  case REQUEST:
    localInfo = (deLocalInfo*) local.addr;
    result.setAddr(localInfo->nextResultTuple());

    return result.addr != 0 ? YIELD : CANCEL;

  case CLOSE:
    qp->Close(args[0].addr);
    qp->Close(args[1].addr);

    localInfo = (deLocalInfo*) local.addr;
    delete localInfo;
    local.addr = 0;
    return 0;
  }
  return 0;
}

/*
2.3 Auxiliary Functions of Operator ~doubleexport~

*/

deLocalInfo::deLocalInfo(Word _streamA, Word wAttrIndexA,
                         Word _streamB, Word wAttrIndexB,
                         Supplier s):resultTupleType(0)
{
  streamA = _streamA;
  streamB = _streamB;
  attrIndexA = StdTypes::GetInt( wAttrIndexA ) - 1;
  attrIndexB = StdTypes::GetInt( wAttrIndexB ) - 1;
  isAEnd = false;

  ListExpr resultType = GetTupleResultType(s);
  resultTupleType = new TupleType( nl->Second( resultType ) );
}

/*
Get tuples from streamA first, and set their ~SI~ as 1.
After traverse the tuples in streamA, get all tuples from streamB,
and set their ~SI~ as 2.

*/

Tuple* deLocalInfo::nextResultTuple()
{
  Tuple* tuple = 0;

  if(!isAEnd){
    tuple = makeTuple(streamA, attrIndexA, 1);
    if (tuple == 0)
      isAEnd = true;
    else
      return tuple;
  }
  tuple = makeTuple(streamB, attrIndexB, 2);
  return tuple;
}

Tuple* deLocalInfo::makeTuple(Word stream, int index,int SI)
{
  bool yield = false;
  Word result;
  Tuple *oldTuple, *newTuple = 0;

  qp->Request(stream.addr, result);
  yield = qp->Received(stream.addr);

  if (yield){
    //Get a tuple from the stream;
    oldTuple = static_cast<Tuple*>(result.addr);

    string key =
        ((Attribute*)(oldTuple->GetAttribute(index)))->getCsvStr();
    string tupStr = oldTuple->WriteToBinStr();
    stringstream vs;
    vs << "(" << SI << " '" << tupStr << "')";

    newTuple = new Tuple(resultTupleType);
    newTuple->PutAttribute(0,new CcString(key));
    newTuple->PutAttribute(1,new FText(true, vs.str()));

    oldTuple->DeleteIfAllowed();
  }

  return newTuple;
}

string binEncode(ListExpr nestList)
{
  stringstream iss, oss;
  nl->WriteBinaryTo(nestList, iss);
  Base64 b64;
  b64.encodeStream(iss, oss);
  string valueStr = oss.str();
  valueStr = replaceAll(valueStr, "\n", "");
  return valueStr;
}

ListExpr binDecode(string binStr)
{
  Base64 b64;
  stringstream iss, oss;
  ListExpr nestList;
  iss << binStr;
  b64.decodeStream(iss, oss);
  nl->ReadBinaryFrom(oss, nestList);
  return nestList;
}

/*
3 Operator ~parahashjoin~

Operator ~parahashjoin~ is used to execute Cartesian product for
a serious of tuples from two different relations
grouped by their join attribute value already
but mixed together in (key, value) schema from Hadoop.

Together with ~doubleexport~ operator, Hadoop has already automatically
finish the hash partition period of a hash join, the tuples
from different source relations but have a same join attribute value,
i.e. inside a same hash bucket will be processed in one reduce function.
However, the number of the tuples inside one hash bucket may be
very small, calling Secondo every time in reduce functions just to process
a few number of tuples is not an efficient solution.
Therefore, in the reduce function, we only send
the tuples into Secondo, and invoke Secondo only once to process the
join operation at last. ~parahashjoin~ is the operator created
to execute the last operation.

At the same time, since the keys that Hadoop uses to partition
tuples into different hash buckets are useless in reduce functions,
they will be abandoned, and only the value parts of the tuples outputed
from ~doubleexport~ operation will be sent into Secondo following
the schema: ((SI + tupleVal) :text).
The ~SI~ is the key field, the ~tupleVal~ is the complete value
of the source tuple in Base 64 code.
And we encapsulate these two value into one text value.

If we only simply send this kind of tuples back to Secondo,
the tuples with different join attributes will be mixed again,
though they have already been grouped automatically by Hadoop.
For avoiding this, in reduce functions,
we send ~OTuple~s whose ~SI~ value is 0 to
separate different hash buckets.

After above procedure, ~parahashjoin~ can easily get tuples
inside one hash bucket with the help of ~OTuple~.
For each hash bucket, ~parahashjoin~ use the key field ~SI~ to
distinguish tuples from different source relations.
Then since all tuples inside have a same join attribute value already,
a simple Cartesian product is caculated for these distinguished tuples.

3.1 Specification of Operator ~parahashjoin~

*/

struct paraHashJoinInfo : OperatorInfo
{
  paraHashJoinInfo()
  {
    name = "parahashjoin";
    signature =
        "stream(tuple((key:int) (value:text)))"
        "x (rel(tuple((a1 t1) ... (an tn))))"
        "x (rel(tuple((b1 p1) ... (bm pm))))"
        "-> stream(tuple((a1 t1) ... (an tn)(b1 p1) ... (bm pm)))";
    syntax = "_ _ _ parahashjoin";
    meaning = "Execute join on a hash partitioned relation";
  }
};

/*
3.1 Type Mapping of Operator ~parahashjoin~

---- ((stream (tuple((key:int) (value:text))))
       x (rel(tuple((a1 t1) ... (an tn))))
       x (rel(tuple((b1 p1) ... (bm pm))))
     -> stream(tuple((a1 t1) ... (an tn)(b1 p1) ... (bm pm))))
----

*/
ListExpr paraHashJoinTypeMap(ListExpr args)
{

  if (nl->ListLength(args) != 3)
  {
    ErrorReporter::ReportError(
      "Operator parahashjoin expect a list of three arguments");
    return nl->TypeError();
  }

  ListExpr stream = nl->First(args);
  ListExpr relA = nl->Second(args);
  ListExpr relB = nl->Third(args);

  if (listutils::isTupleStream(stream)
    && listutils::isRelDescription(relA)
    && listutils::isRelDescription(relB))
  {
    ListExpr streamTupleList = nl->Second(nl->Second(stream));
    if (nl->ListLength(streamTupleList) != 1)
    {
      ErrorReporter::ReportError(
        "Operator parahashjoin only accept tuple stream "
        "with one TEXT type argument");
      return nl->TypeError();
    }
    else if (!listutils::isSymbol(
        nl->Second(nl->First(streamTupleList)),TEXT))
    {
      ErrorReporter::ReportError(
              "Operator parahashjoin only accept tuple stream "
              "with one TEXT type argument");
      return nl->TypeError();
    }

    ListExpr rAtupNList =
        renameList(nl->Second(nl->Second(relA)), "1");
    ListExpr rBtupNList =
        renameList(nl->Second(nl->Second(relB)), "2");
    ListExpr resultAttrList = ConcatLists(rAtupNList, rBtupNList);
    ListExpr resultList = nl->TwoElemList(nl->SymbolAtom("stream"),
          nl->TwoElemList(nl->SymbolAtom("tuple"), resultAttrList));

    return resultList;

  }
  else
  {
    ErrorReporter::ReportError(
      "Operator parahashjoin expect input as "
        "stream(tuple) x rel(tuple) x rel(tuple)");
    return nl->TypeError();
  }

}

/*
Rename the attributes in both relations to avoid duplication of names.

*/

ListExpr renameList(ListExpr oldTupleList, string appendName)
{
  NList newList;
  ListExpr rest = oldTupleList;
  while(!nl->IsEmpty(rest)){
    ListExpr tuple = nl->First(rest);
    string attrname = nl->SymbolValue(nl->First(tuple));
    attrname.append("_" + appendName);

    NList newTuple(nl->TwoElemList(
                    nl->SymbolAtom(attrname),
                    nl->Second(tuple)));
    newList.append(newTuple);
    rest = nl->Rest(rest);
  }
  return newList.listExpr();
}

/*
3.2 Value Mapping of Operator ~parahashjoin~

*/
int paraHashJoinValueMap(Word* args, Word& result,
                int message, Word& local, Supplier s)
{

  phjLocalInfo *localInfo;
  ListExpr aTupleTypeList, bTupleTypeList;

  switch (message)
  {
  case OPEN:
    qp->Open(args[0].addr);

    aTupleTypeList =
        SecondoSystem::GetCatalog()->NumericType(
        nl->Second(qp->GetSupplierTypeExpr(qp->GetSon(s,1))));
    bTupleTypeList =
        SecondoSystem::GetCatalog()->NumericType(
        nl->Second(qp->GetSupplierTypeExpr(qp->GetSon(s,2))));

    localInfo = new phjLocalInfo(args[0], s,
        aTupleTypeList, bTupleTypeList);
    local = SetWord(localInfo);
    return 0;
  case REQUEST:
    localInfo = (phjLocalInfo*) local.addr;
    result = localInfo->nextJoinTuple();

    return result.addr !=0 ? YIELD : CANCEL;
  case CLOSE:
    qp->Close(args[0].addr);

    localInfo = (phjLocalInfo*) local.addr;
    delete localInfo;
    localInfo = 0;
    return 0;
  }
  return 0;
}

/*
3.3 Auxiliary Functions of Operator ~parahashjoin~

*/

phjLocalInfo::phjLocalInfo(Word _stream, Supplier s,
    ListExpr ttA, ListExpr ttB)
{
  mixStream = _stream;

  ListExpr resultType = GetTupleResultType(s);
  resultTupleType = new TupleType(nl->Second(resultType));

  tupleTypeA = new TupleType(ttA);
  tupleTypeB = new TupleType(ttB);

  joinedTuples = 0;
  tupleIterator = 0;
}

/*
Ask for new tuples from ~joinedTuples~.
If there's no more tuples inside ~joinedTuples~,
then invoke ~getNewProducts~ to get new results.

*/
Word phjLocalInfo::nextJoinTuple()
{
  Tuple *tuple;

  if (tupleIterator != 0)
  {
    if ((tuple = tupleIterator->GetNextTuple()) != 0)
      return SetWord(tuple);
    else
    {
      delete tupleIterator;
      tupleIterator = 0;
    }
  }

  if ((tupleIterator = getNewProducts()) != 0)
  {
    tuple = tupleIterator->GetNextTuple();
    return SetWord(tuple);
  }

  return SetWord(Address(0));
}


/*
Collect and distinguish tuples of one bucket.
If the key field value, i.e. ~SI~ is 1, means the tuple comes from rel1,
and if ~SI~ is 2, then means the tuple comes from rel2.
Besides, if ~SI~ is 0, then the tuple is the separator tuple(~ST~).

If the tuples of one bucket all come from a same source relation,
then jump to next bucket because there will be no product results in
this bucket.
Or else, make the products, and put the result tuples into the ~joinedTuples~.

*/

GenericRelationIterator* phjLocalInfo::getNewProducts()
{

  TupleBuffer *tbA = 0;
  TupleBuffer *tbB = 0;
  GenericRelationIterator *iteratorA = 0, *iteratorB = 0;
  Tuple *tupleA = 0, *tupleB = 0;
  string tupStr, sTupStr;
  long MaxMem = qp->MemoryAvailableForOperator();

  //  Traverse the stream, until there is no more tuples exists,
  //  or the ~joinedTuples~ is filled.
  while(true)
  {
    tbA = new TupleBuffer(MaxMem);
    tbB = new TupleBuffer(MaxMem);
    //  Collect tuples in one bucket.
    Word currentTupleWord(Address(0));
    bool isInBucket = true;
    qp->Request(mixStream.addr, currentTupleWord);

    while(qp->Received(mixStream.addr))
    {
      Tuple* currentTuple =
          static_cast<Tuple*> (currentTupleWord.addr);
      tupStr =
          ((FText*) (currentTuple->GetAttribute(0)))->GetValue();
      currentTuple->DeleteIfAllowed();

      int SI = atoi(tupStr.substr(1,1).c_str());
      sTupStr = tupStr.substr(4, tupStr.size() - 6);

      switch (SI)
      {
      case 1:{
        tupleA = new Tuple(tupleTypeA);
        tupleA->ReadFromBinStr(sTupStr);
        tbA->AppendTuple(tupleA);
        tupleA->DeleteIfAllowed();
        break;
      }
      case 2:{
        tupleB = new Tuple(tupleTypeB);
        tupleB->ReadFromBinStr(sTupStr);
        tbB->AppendTuple(tupleB);
        tupleB->DeleteIfAllowed();
        break;
      }
      case 0:{
        isInBucket = false;
        break;
      }
      default:{
        //should never be here
        cerr << "Exist tuples with error SI value" << endl;
        assert(false);
      }
      }

      if (isInBucket)
        qp->Request(mixStream.addr, currentTupleWord);
      else
        break;
    }

    int countA = tbA->GetNoTuples();
    int countB = tbB->GetNoTuples();

    if(countA == 0 && countB == 0)
    {
      // No more data exists
      delete tbA;
      delete tbB;
      return false;
    }
    else if(countA == 0 || countB == 0)
    {
      // All tuples come from one source relation
      delete tbA;
      delete tbB;
    }
    else
    {
      //compute the products
      if (joinedTuples != 0)
        delete joinedTuples;
      joinedTuples = new TupleBuffer(MaxMem);

      int i = 0, j = 0;
      iteratorA = tbA->MakeScan();
      tupleA = iteratorA->GetNextTuple();
      while(tupleA && i++ < countA)
      {
        j = 0;
        iteratorB = tbB->MakeScan();
        tupleB = iteratorB->GetNextTuple();
        while(tupleB && j++ < countB)
        {
          Tuple *resultTuple = new Tuple(resultTupleType);
          Concat(tupleA, tupleB,resultTuple);
          tupleB->DeleteIfAllowed();

          joinedTuples->AppendTuple(resultTuple);
          resultTuple->DeleteIfAllowed();
          tupleB = iteratorB->GetNextTuple();
        }
        delete iteratorB;
        tupleA->DeleteIfAllowed();
        tupleA = iteratorA->GetNextTuple();
      }
      delete iteratorA;

      delete tbA;
      delete tbB;

      return joinedTuples->MakeScan();
    }
  }
  return 0;
}

/*
4. Type Operator ~TUPSTREAM~

This type operator extract the type of the element from a rel type
given as the first argument,
and forwards this type encapsulated in a stream type.

----
    ( (rel(T1)) ... ) -> stream(T1)
----

4.1 Specification of Operator ~TUPSTREAM~

*/

struct TUPSTREAMInfo : OperatorInfo
{
  TUPSTREAMInfo()
  {
    name = "TUPSTREAM";
    signature =
        "( (rel(T1)) ... ) -> stream(T1)";
    syntax = "type operator";
    meaning = "Extract the tuple of a relation "
        "from the first argument, and forward it as a stream";
  }
};

ListExpr TUPSTREAMType( ListExpr args)
{
  ListExpr first;
  CHECK_COND(nl->ListLength(args) >= 1,
      "Expect one argument at least");
  first = nl->First(args);
  CHECK_COND(listutils::isRelDescription(first),
      "rel(tuple(...)) expected");
  return nl->TwoElemList(nl->SymbolAtom(STREAM), nl->Second(first));
}


/*
4. Type Operator ~TUPSTREAM2~

This type operator extract the type of the element from a rel type
given as the second argument,
and forwards this type encapsulated in a stream type.

----
    ( T1 (rel(T2)) ... ) -> stream(T2)
----

4.1 Specification of Operator ~TUPSTREAM2~

*/

struct TUPSTREAM2Info : OperatorInfo
{
  TUPSTREAM2Info()
  {
    name = "TUPSTREAM2";
    signature =
        "( T1 (rel(T2)) ... ) -> stream(T2)";
    syntax = "type operator";
    meaning = "Extract the tuple of a relation "
        "from the second argument, and forward it as a stream";
  }
};

ListExpr TUPSTREAM2Type( ListExpr args)
{
  ListExpr second;
  CHECK_COND(nl->ListLength(args) >= 2,
      "Expect two argument at least");
  second = nl->Second(args);
  CHECK_COND(listutils::isRelDescription(second),
      "rel(tuple(...)) expected");
  return nl->TwoElemList(nl->SymbolAtom(STREAM), nl->Second(second));
}


/*
4. Type Operator ~TUPSTREAM3~

This type operator extract the type of the element from a rel type
given as the third argument,
and forwards this type encapsulated in a stream type.

----
    ( T1 T2 (rel(T3)) ... ) -> stream(T3)
----

4.1 Specification of Operator ~TUPSTREAM3~

*/

struct TUPSTREAM3Info : OperatorInfo
{
  TUPSTREAM3Info()
  {
    name = "TUPSTREAM3";
    signature =
        "( T1 T2 (rel(T3)) ... ) -> stream(T3)";
    syntax = "type operator";
    meaning = "Extract the tuple of a relation "
        "from the third argument, and forward it as a stream";
  }
};

ListExpr TUPSTREAM3Type( ListExpr args)
{
  ListExpr third;
  CHECK_COND(nl->ListLength(args) >= 1,
      "Expect one argument at least");
  third = nl->Third(args);
  CHECK_COND(listutils::isRelDescription(third),
      "rel(tuple(...)) expected");
  return nl->TwoElemList(nl->SymbolAtom(STREAM), nl->Second(third));
}

/*
5 Operator ~parajoin~

Operator ~parahashjoin~ can only execute ~product~ operation for the
tuples belong to different source relation but inside
a same hash bucket.
However, for some specific join operations like spatial operation,
tuples inside one bucket don't means they have an exactly same
join attribute value, and so does the Cartesian product can't be
executed directly for these tuples.

At the same time, ~parahashjoin~ is inefficient for some big join
operations, since we store all result tuples into a temporal
tupleBuffer which will visit the disk if the amount of the
result tuples is too large.

Therefore, we need to create the operator ~parajoin~ that can process
the tuples inside one hash bucket but with different join operations.
Similar with ~parahashjoin~, ~parajoin~ accept the stream mixed with
tuples following two different schemes. These tuples are
partitioned into different buckets according to their join
attribute values, and use ~0Tuple~s to separate these
buckets. At the same time, each tuple contains a ~SI~ value to
indicate which source relations it comes from or dose it a ~OTuple~.
With the ~SI~ values, the operator can get all tuples in one bucket,
and distinguish them into two tuple buffers.

The difference of ~parajoin~ between ~parahashjoin~ is that it can
accept any kind of join operator as its parameter function,
and use this function to execute different join operations for the
tuples inside one hash bucket.
The type of operators can be accepted in ~parajoin~ should be like:

---- stream(T1) x stream(T2) -> stream(T3)
----

The main problem here is that
the function should accept two streams as input, and output
a stream, which doesn't like normal functions which only can
accept DATA object and output DATA or stream.
But thanks to the PartittionedStream algebra, it modify the kernel of
Secondo, and make this kind of function be possible.

For making the functions be possible to accept two streams as input,
we can store the supplier of this operator at the tail two positions
of this function's argument list. Then the query processor knows that
these two inputs are streams, and will use specific messages to drive
the function work.

*/

struct paraJoinInfo : OperatorInfo
{
  paraJoinInfo()
  {
    name = "parajoin";
    signature =
        "( (stream(tuple((key int)(value text))))"
        "x(rel(tuple(T1))) x (rel(tuple(T2)))"
        "x(map (stream(T1)) (stream(T2)) (stream(T1 T2))) )"
        " -> stream(tuple(T1 T2))";
    syntax = "_ _ _ parajoin [funlist]";
    meaning = "join mixed tuples from two relations";
  }
};

/*
5.1 Type Mapping of Operator ~parajoin~

----
    (  (stream(tuple((value text))))
     x (rel(tuple(T1))) x (rel(tuple(T2)))
     x ((map (stream(T1)) (stream(T2)) (stream(T1 T2))))  )
     -> stream(tuple(T1 T2))
----

*/

ListExpr paraJoinTypeMap( ListExpr args )
{
  if (nl->ListLength(args) != 4)
  {
    ErrorReporter::ReportError(
      "Operator parajoin expect a list of four arguments");
    return nl->TypeError();
  }

  ListExpr streamList = nl->First(args);
  ListExpr relAList = nl->Second(args);
  ListExpr relBList = nl->Third(args);
  ListExpr mapNL = nl->Fourth(args);

  if (listutils::isTupleStream(streamList)
    && listutils::isRelDescription(relAList)
    && listutils::isRelDescription(relBList))
  {
    ListExpr attrList = nl->Second(nl->Second(streamList));
    if (nl->ListLength(attrList) != 1)
    {
      ErrorReporter::ReportError(
        "Operator parajoin only accept tuple stream "
        "with one TEXT type argument");
      return nl->TypeError();
    }
    else if (!listutils::isSymbol(
      nl->Second(nl->First(attrList)),TEXT))
    {
      ErrorReporter::ReportError(
        "Operator parajoin only accept tuple stream "
        "with one TEXT type argument");
      return nl->TypeError();
    }

    if (listutils::isMap<2>(mapNL))
    {
      if (listutils::isTupleStream(nl->Second(mapNL))
        && listutils::isTupleStream(nl->Third(mapNL))
        && listutils::isTupleStream(nl->Fourth(mapNL)))
      {
        ListExpr resultList = nl->TwoElemList(
              nl->SymbolAtom("stream"),
              nl->TwoElemList(nl->SymbolAtom("tuple"),
                  nl->Second(nl->Second(nl->Fourth(mapNL)))));

        return resultList;
      }
      else
      {
        ErrorReporter::ReportError(
            "Operator parajoin expects parameter function "
            "as (map (stream(T1)) (stream(T2)) (stream(T1 T2)))");
        return nl->TypeError();
      }
    }
    else
    {
      ErrorReporter::ReportError(
        "Operator parajoin expects binary function "
          "as the fourth argument.");
      return nl->TypeError();
    }
  }
  else
  {
    ErrorReporter::ReportError(
      "Operator parajoin expect "
        "(stream(tuple((value text))))"
        "x(rel(tuple(T1))) x (rel(tuple(T2)))"
        "x((map (stream(T1)) (stream(T2)) (stream(T1 T2))))");
    return nl->TypeError();
  }
}


/*
5.2 Value Mapping of Operator ~parajoin~

Here the message like ~(1[*]FUNMSG)+OPEN~ means the function
needs to open its first stream, and ~(1[*]FUNMSG)+REQUEST~ means
the function needs to request its first stream, and so do other
similar messages.

*/
int paraJoinValueMap(Word* args, Word& result,
                int message, Word& local, Supplier s)
{

  pjLocalInfo *localInfo;
  ListExpr aTupleTypeList, bTupleTypeList;

  switch (message)
  {
  case OPEN:{
    qp->Open(args[0].addr);

    aTupleTypeList =
        SecondoSystem::GetCatalog()->NumericType(
        nl->Second(qp->GetSupplierTypeExpr(qp->GetSon(s,1))));
    bTupleTypeList =
        SecondoSystem::GetCatalog()->NumericType(
        nl->Second(qp->GetSupplierTypeExpr(qp->GetSon(s,2))));

    localInfo = new pjLocalInfo(args[0], args[3].addr, s,
        aTupleTypeList, bTupleTypeList,
        qp->MemoryAvailableForOperator());

    local.setAddr(localInfo);
    return 0;
  }
  case REQUEST:{
    // ask the fun to get the result tuple.
    if (local.addr == 0)
      return CANCEL;
    localInfo = (pjLocalInfo*) local.addr;

    result.setAddr(localInfo->getNextTuple());
    if (result.addr)
      return YIELD;
    else
      return CANCEL;
  }
  case (1*FUNMSG)+OPEN:{
    return 0;
  }
  case (2*FUNMSG)+OPEN:{
    return 0;
  }
  case (1*FUNMSG)+REQUEST:{
    if (local.addr == 0)
      return CANCEL;
    localInfo = (pjLocalInfo*) local.addr;

    result.setAddr(localInfo->getNextInputTuple(tupBufferA));
    if ( result.addr != 0)
      return YIELD;
    else
      return CANCEL;
  }
  case (2*FUNMSG)+REQUEST:{
    if (local.addr == 0)
      return CANCEL;
    localInfo = (pjLocalInfo*) local.addr;

    result.setAddr(localInfo->getNextInputTuple(tupBufferB));
    if ( result.addr != 0)
      return YIELD;
    else
      return CANCEL;
  }
  case (1*FUNMSG)+CLOSE:{
    return 0;
  }
  case (2*FUNMSG)+CLOSE:{
    return 0;
  }
  case CLOSE:{
    if (local.addr == 0)
      return CANCEL;
    localInfo = (pjLocalInfo*) local.addr;

    delete localInfo;
    qp->Close(args[0].addr);
    return 0;
  }
  }

  return 0;
}

/*
3.3 Auxiliary Functions of Operator ~parajoin~

Load one bucket tuples from the input tuple stream,
and fill them into two different tupleBuffers according to the
~SI~ value it contains.
If the tuples in that bucket all come from one source relation,
then move to the next bucket directly.

*/
void pjLocalInfo::loadTuples()
{
  if (endOfStream)
  {
    cerr << "The input mixed stream is exhausted." << endl;
    return;
  }

  Word cTupleWord(Address(0));
  bool isInBucket;
  Tuple *cTuple = 0;
  Tuple *tupleA = 0, *tupleB = 0;
  string tupStr, sTupStr;

  if (itrA != 0)
    delete itrA;
  itrA = 0;
  if(tbA != 0)
    delete tbA;
  tbA = 0;

  if (itrB != 0)
    delete itrB;
  itrB = 0;
  if(tbB != 0)
    delete tbB;
  tbB = 0;

  while (!endOfStream)
  {
    tbA = new TupleBuffer(maxMem);
    tbB = new TupleBuffer(maxMem);
    isBufferFilled = false;
    isInBucket = true;

    qp->Request(mixedStream.addr, cTupleWord);
    while (isInBucket && qp->Received(mixedStream.addr))
    {
      cTuple = static_cast<Tuple*> (cTupleWord.addr);
      tupStr = ((FText*) (cTuple->GetAttribute(0)))->GetValue();

      int SI = atoi(tupStr.substr(1,1).c_str());
      sTupStr = tupStr.substr(4, tupStr.size() - 6);

      switch (SI)
      {
      case 1:
      {
        tupleA = new Tuple(tupleTypeA);
        tupleA->ReadFromBinStr(sTupStr);
        tbA->AppendTuple(tupleA);
        tupleA->DeleteIfAllowed();
        break;
      }
      case 2:
      {
        tupleB = new Tuple(tupleTypeB);
        tupleB->ReadFromBinStr(sTupStr);
        tbB->AppendTuple(tupleB);
        tupleB->DeleteIfAllowed();
        break;
      }
      case 0:
      {
        isInBucket = false;
        break;
      }
      default:
      {
        //should never be here
        cerr << "Exist tuples with error SI value" << endl;
        assert(false);
      }
      }

      cTuple->DeleteIfAllowed();
      if (isInBucket)
        qp->Request(mixedStream.addr, cTupleWord);
    }

    int numOfA = tbA->GetNoTuples();
    int numOfB = tbB->GetNoTuples();

    if (numOfA == 0 && numOfB == 0)
    {
      delete tbA;
      delete tbB;
      tbA = tbB = 0;
      endOfStream = true;
      break;
    }
    else if (numOfA == 0 || numOfB == 0)
    {
      delete tbA;
      delete tbB;
      tbA = tbB = 0;
    }
    else
    {
      tpIndex_A = tpIndex_B = 0;
      isBufferFilled = true;
      itrA = tbA->MakeScan();
      itrB = tbB->MakeScan();
      break;
    }
  }

}

/*
Take one tuple from tupleBuffer A or B.
When the operator in the parameter function need one tuple
from the input stream, it gets the tuple from the
filled tuple buffer actually. When both tuple buffers are exhausted,
then continue scan the input stream until the input stream is
exhausted too.

*/
Tuple* pjLocalInfo::getNextInputTuple(tupleBufferType tbt)
{
  Tuple* tuple = 0;

  if(itrA && tbt == tupBufferA){
    tuple = itrA->GetNextTuple();
  }
  else if (itrB){
    tuple = itrB->GetNextTuple();
  }

  return tuple;
}

/*
While the input stream is not exhausted,
keep asking the function to get one result.
If the function's output stream is exhausted,
then load the tuples of one bucket from the input stream.

*/
void* pjLocalInfo::getNextTuple()
{
  Word funResult(Address(0));

  while (!endOfStream)
  {
    qp->Request(JNfun, funResult);
    if (funResult.addr){
      return funResult.addr;
    }
    else if (endOfStream) {
      qp->Close(JNfun);
      return 0;
    }
    else {
      // No more result in current bucket, load the next bucket
      qp->Close(JNfun);
      loadTuples();
      if (isBufferFilled)
        qp->Open(JNfun);
      continue;
    }
  }
  return 0;
}

/*
6 Operator ~add0Tuple~

The tuples outputed from ~doubleexport~ can't be used directly by
~parahashjoin~ or ~parajoin~, because the MapReduce job is needed
to sort these tuples according to their join attribute values,
and add those ~0Tuple~s to partition those tuples into different
buckets.

For simulating this proceduce in Secondo, we create this operator
called ~add0Tuple~.
This operator must get the outputs from ~doubleexport~, and be used
after a ~sortby~ operator which sort the tuples by their keys.
Then this operator can scan the whole stream, and add the ~0Tuple~s
when the keys values change.

At the same time, this operator also abandon the keyT field of
the input stream, only extract the valueT field to the next operator,
like ~parahashjoin~ or ~parajoin~.

Added in 21th July 2010 -- Jiamin
I changed the ~add0Tuple~ to keep the ~keyT~ attribute,
to reduce the additional overhead of creating new tuples.
Therefore, a ~project~ operator is needed to project the ~valueT~ part
only to the following ~parajoin~ or ~parahashjoin~ operator.

*/

struct add0TupleInfo : OperatorInfo
{
  add0TupleInfo()
  {
    name = "add0Tuple";
    signature =
        "(  (stream(tuple((keyT string)(valueT text))))"
        "-> stream(tuple((keyT string)(valueT text)))  )";
    syntax = "_ add0Tuple";
    meaning = "Separate tuples by inserting 0 tuples";
  }
};

/*
6.1 Type Mapping of Operator ~add0Tuple~

----
    (stream(tuple((keyT string)(valueT text))))
      -> stream(tuple((keyT string)(valueT text)))
----

*/
ListExpr add0TupleTypeMap(ListExpr args)
{
  int len = nl->ListLength(args);
  if (len != 1)
  {
    ErrorReporter::ReportError(
            "Operator add0TupleTypeMap only expect one argument.");
    return nl->TypeError();
  }

  ListExpr streamNL = nl->First(args);
  if (!listutils::isTupleStream(streamNL))
  {
    ErrorReporter::ReportError(
            "Operator add0TupleTypeMap expect a tuple stream.");
    return nl->TypeError();
  }

  ListExpr tupleList = nl->Second(nl->Second(streamNL));
  if (nl->ListLength(tupleList) == 2
  && listutils::isSymbol(nl->Second(nl->First(tupleList)), STRING)
  && listutils::isSymbol(nl->Second(nl->Second(tupleList)), TEXT))
  {
    return streamNL;
//    return nl->TwoElemList(nl->SymbolAtom(STREAM),
//          nl->TwoElemList(nl->SymbolAtom(TUPLE),
//              nl->OneElemList(nl->Second(tupleList))));
  }
  else
  {
    ErrorReporter::ReportError(
           "Operator add0TupleTypeMap expect input "
           "as stream(tuple((string)(text)))");
    return nl->TypeError();
  }
}


/*
6.2 Value Mapping of Operator ~add0Tuple~


*/
int add0TupleValueMap(Word* args, Word& result,
                      int message, Word& local, Supplier s)
{
  a0tLocalInfo *localInfo;
  Word cTupleWord;
  Tuple *oldTuple, *sepTuple;

  switch (message)
  {
  case OPEN:{
    qp->Open(args[0].addr);

    ListExpr resultTupleNL = GetTupleResultType(s);
    localInfo = new a0tLocalInfo(resultTupleNL);

    local.setAddr(localInfo);
    return 0;
  }
  case REQUEST:{
    if (local.addr == 0)
      return CANCEL;
    localInfo = (a0tLocalInfo*)local.addr;

    if(localInfo->needInsert)
    {
      // Output the cached tuple
      result.setAddr(localInfo->cachedTuple);

      localInfo->cachedTuple = 0;
      localInfo->needInsert = false;
      return YIELD;
    }
    else
    {
      qp->Request(args[0].addr, cTupleWord);
      if (qp->Received(args[0].addr))
      {
        oldTuple = (Tuple*)cTupleWord.addr;
        string key =
            ((CcString*)(oldTuple->GetAttribute(0)))->GetValue();

        if ("" == localInfo->key)
          localInfo->key = key;  //Set the initial key value

        if (key == localInfo->key)
        {
          result.setAddr(oldTuple);  //Unchanged key value
          return YIELD;
        }
        else
        {
          //  The key value changes,
          //  cache the current tuple with changed key,
          //  and insert the separate 0Tuple
          localInfo->cachedTuple = oldTuple;
          localInfo->needInsert = true;
          localInfo->key = key;

          sepTuple = new Tuple(localInfo->resultTupleType);
          sepTuple->PutAttribute(0, new CcString(true, "0Tuple"));
          sepTuple->PutAttribute(1, new FText(true, "(0 '')"));

          result.setAddr(sepTuple);
          return YIELD;
        }
      }
      else
        return CANCEL;
    }
  }
  case CLOSE:{
    if (local.addr == 0)
      return CANCEL;
    localInfo = (a0tLocalInfo*)local.addr;
    delete localInfo;
    local.setAddr(0);
    qp->Close(args[0].addr);
    return 0;
  }
  }
  return 0;
}

/*
5.15 Operator ~fconsume~

This operator maps

----   stream(tuple(...)) x string x [int] -> bool
----

This operator writes the tuples of the accepted tuple-stream
into a binary file. The name of the file depends on the second
accepted argument ~fileName~. If the third argument ~index~ is
available, then the file name will add a postfix with this ~index~,
to distinguish this file from a group of similar files.

Besides, the nested list of the relation's type is written into
a separated file but in a same directory. Use a separated file to
put the type information is because the type mapping function of
the below operator ~ffeed~ needs to read this file,
and return the type information while importing the binary file.


5.15.0 Specification

*/

struct FConsumeInfo : OperatorInfo {

  FConsumeInfo() : OperatorInfo()
  {
    name =      "fconsume";
    signature = "stream(tuple(...)) x string x [int] -> bool";
    syntax =    "_ fconsume[ _ , _ ]";
    meaning =   "write a stream of tuples into a binary file";
  }

};

/*
5.14.1 Type mapping

*/
ListExpr FConsumeTypeMap(ListExpr args)
{
  NList l(args);
  string err = "operator fconsume expects "
               "(stream(tuple(...)), string, [int])";

  if(l.length() < 2 || l.length() > 3)
    return l.typeError(err);

  NList attr;
  if(!l.first().checkStreamTuple(attr) )
    return l.typeError(err);

  if(!l.second().isSymbol(Symbols::STRING()) )
    return l.typeError(err);

  if(l.length() == 3 && !l.third().isSymbol(Symbols::INT()) )
    return l.typeError(err);

  return NList(Symbols::BOOL()).listExpr();
}
/*
5.14.2 Value mapping

*/
struct fconsumeLocalInfo
{
  int state;
  int current;
};

int FConsumeValueMap(Word* args, Word& result,
    int message, Word& local, Supplier s)
{
  fconsumeLocalInfo* fcli;

  if ( message <= CLOSE)
  {
    result = qp->ResultStorage(s);

    fcli = (fconsumeLocalInfo*) local.addr;
    if (fcli) delete fcli;

    fcli = new fconsumeLocalInfo();
    fcli->state = 0;
    fcli->current = 0;
    local.setAddr(fcli);

    int index = -1;
    string relName;
    if (qp->GetNoSons(s) == 3 )
      index = ((CcInt*)args[2].addr)->GetIntval();

    relName = ((CcString*)args[1].addr)->GetValue();

    //Write the type of the relation into a separated file.
    string typeFileName = FileSystem::GetCurrentFolder();
    FileSystem::AppendItem(typeFileName, "cell");
    FileSystem::AppendItem(typeFileName, relName + "_type");
    ofstream typeFile(typeFileName.c_str());
    ListExpr relTypeList;
    if (!typeFile.good())
    {
      cerr << "Create typeInfo file"
          << relName + "_type" << " error!\n";
      ((CcBool*)(result.addr))->Set(true, false);
      return 0;
    }
    else
    {
      //The accepted input is a stream tuple
      relTypeList = nl->TwoElemList(
        nl->SymbolAtom("rel"),
        nl->Second(qp->GetSupplierTypeExpr(qp->GetSon(s,0))) );
      typeFile << nl->ToString(relTypeList) << endl;
      typeFile.close();
    }

    //Write complete tuples into a binary file.
    if (index > 0)
    {
      stringstream ss;
      ss << relName << "_" << index;
      relName = ss.str();
    }
    //create a path for this file.
    string blockFileName = FileSystem::GetCurrentFolder();
    FileSystem::AppendItem(blockFileName, "cell");
    FileSystem::AppendItem(blockFileName, relName);

    ofstream blockFile(blockFileName.c_str(), ios::binary);
    if (!blockFile.good())
    {
      cerr << "Create file " << blockFileName << " error!" << endl;
      ((CcBool*)(result.addr))->Set(true, false);
      return 0;
    }

    //Statistic information
    TupleType *tt = new TupleType(SecondoSystem::GetCatalog()
                        ->NumericType(nl->Second(relTypeList)));
    vector<double> attrExtSize(tt->GetNoAttributes());
    vector<double> attrSize(tt->GetNoAttributes());
    double totalSize = 0.0;
    double totalExtSize = 0.0;
    int count = 0;

    Word wTuple(Address(0));
    qp->Open(args[0].addr);
    qp->Request(args[0].addr, wTuple);
    while(qp->Received(args[0].addr))
    {
      Tuple* t = static_cast<Tuple*>(wTuple.addr);

      size_t coreSize = 0;
      size_t extensionSize = 0;
      size_t flobSize = 0;
      size_t tupleBlockSize =
          t->GetBlockSize(coreSize, extensionSize, flobSize,
                          &attrExtSize, &attrSize);

      totalSize += (coreSize + extensionSize + flobSize);
      totalExtSize += (coreSize + extensionSize);

      char* tBlock = (char*)malloc(tupleBlockSize);
      t->WriteToBin(tBlock, coreSize, extensionSize, flobSize);
      blockFile.write(tBlock, tupleBlockSize);
      free(tBlock);
      count++;
      fcli->current++;

      t->DeleteIfAllowed();
      qp->Request(args[0].addr, wTuple);
    }

    // write a zero after all tuples to indicate the end.
    u_int32_t endMark = 0;
    blockFile.write((char*)&endMark, sizeof(endMark));

    // build a description list of output tuples
    NList descList;
    descList.append(NList(count));
    descList.append(NList(totalExtSize));
    descList.append(NList(totalSize));
    for(int i = 0; i < tt->GetNoAttributes(); i++)
    {
      descList.append(NList(attrExtSize[i]));
      descList.append(NList(attrSize[i]));
    }

    //put the base64 code of the description list to the file end.
    string descStr = binEncode(descList.listExpr());
    u_int32_t descSize = descStr.size() + 1;
    blockFile.write(descStr.c_str(), descSize);
    blockFile.write((char*)&descSize, sizeof(descSize));

    qp->Close(args[0].addr);
    blockFile.close();
    cout << "\nCreate block fileName: " << blockFileName << endl;

    ((CcBool*)(result.addr))->Set(true, true);
    fcli->state = 1;
    return 0;

  }
  else if ( message == REQUESTPROGRESS )
  {
    ProgressInfo p1;
    ProgressInfo* pRes;
    const double uConsume = 0.024;   //millisecs per tuple
    const double vConsume = 0.0003;  //millisecs per byte in
                                        //  root/extension
    const double wConsume = 0.001338;  //millisecs per byte in FLOB

    fcli = (fconsumeLocalInfo*) local.addr;
    pRes = (ProgressInfo*) result.addr;

    if (qp->RequestProgress(args[0].addr, &p1))
    {
      pRes->Card = p1.Card;
      pRes->CopySizes(p1);

      pRes->Time = p1.Time + p1.Card *
            (uConsume + p1.SizeExt * vConsume
             + (p1.Size - p1.SizeExt) * wConsume);

      if ( fcli == 0 )
      {
        pRes->Progress = (p1.Progress * p1.Time) / pRes->Time;
      }
      else
      {
        if (fcli->state == 0)
        {
          if ( p1.BTime < 0.1 && pipelinedProgress ) //non-blocking,
                                                     //use pipelining
            pRes->Progress = p1.Progress;
          else
            pRes->Progress =
            (p1.Progress * p1.Time +
              fcli->current *  (uConsume + p1.SizeExt * vConsume
                  + (p1.Size - p1.SizeExt) * wConsume) )
                / pRes->Time;
        }
        else
        {
          pRes->Progress = 1.0;
        }
      }

      pRes->BTime = pRes->Time;    //completely blocking
      pRes->BProgress = pRes->Progress;

      return YIELD;      //successful
    }
    else
      return CANCEL;
  }
  else if ( message == CLOSEPROGRESS )
  {
    fcli = (fconsumeLocalInfo*) local.addr;
    if ( fcli ){
       delete fcli;
       local.setAddr(0);
    }
    return 0;
  }

  return 0;
}

const string FConsumeSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" ) "
  "( <text>(stream(tuple(...)) x string x [int]) -> "
  "( bool )</text--->"
  "<text>_ fconsume[ _ , _ ]</text--->"
  "<text>Write a stream of tuples into a binary file.</text--->"
  ") )";

Operator fconsumeOp (
    "fconsume",               // name
    FConsumeSpec,             // specification
    FConsumeValueMap,                 // value mapping
    Operator::SimpleSelect, // trivial selection function
    FConsumeTypeMap           // type mapping
);

/*
5.15 Operator ~ffeed~

This operator maps

----   relName x string x [int] -> rel(tuple(...))
----

This operator restore a relation from a binary file
created by ~fconsume~ operator.

The first argument is used to define the name of the relation that we
should read from. It's composed by two parts, prefix ~f\_~ and ~name~.
The prefix is used to specify that the relation is read from a file.

As we explained in ~fconsume~, two files are created to store the tuples,
one is type info file, whose name is ~name~\_type, and another is binary
file, whose name is ~name~ \_index,
where the ~index~ is the third argument of this query.
If the ~index~ doesn't exist, then the file name is only the ~name~ itself.

Besides, the second argument defines the ~path~ of these two files.
If the ~path~ is empty, then the files are put in a default path,
SECONDO\_BUILD\_DIR/bin/cell/, or else the binary file can't be put into
another specified path.

5.15.0 Specification

*/

struct FFeedInfo : OperatorInfo {

  FFeedInfo() : OperatorInfo()
  {
    name =      "ffeed";
    signature = "relName x string x [int] -> stream(tuple(...))";
    syntax =    "ffeed( _, _, _ )";
    meaning =   "restore a relation from a binary file"
                "created by ~fconsume~ operator.";
  }

};

/*
5.14.1 Type mapping

*/
ListExpr FFeedTypeMap(ListExpr args)
{
  NList l(args);
  string err = "relfile expects (f_relName, string, [int])";

  if (l.length() < 2 || l.length() > 3)
    return l.typeError(err);

  if (!(nl->IsAtom(l.first().listExpr())))
    return l.typeError(err);

  string relName = nl->SymbolValue(l.first().listExpr());
  if (relName.find_first_of("f_") > 0)
    return l.typeError(err);
  relName = relName.substr(2);

  if (!l.second().isSymbol(Symbols::STRING()))
    return l.typeError(err);

  if (l.length() == 3 && !l.third().isSymbol(Symbols::INT()))
    return l.typeError(err);

  //The type file can only be put into the default directory
  string typeFileName = FileSystem::GetCurrentFolder();
  FileSystem::AppendItem(typeFileName, "cell");
  FileSystem::AppendItem(typeFileName, relName + "_type");
  ListExpr relType;
  if(!nl->ReadFromFile(typeFileName, relType))
  {
    ErrorReporter::ReportError("Can't open file: " + typeFileName);
    return nl->TypeError();
  }

  if(!listutils::isRelDescription(relType))
  {
      ErrorReporter::ReportError("The nested list in file: "
          + typeFileName + " is not a tuple relation type.");
      return nl->TypeError();
  }

  ListExpr streamType = nl->TwoElemList(
    nl->SymbolAtom("stream"),
    nl->Second(relType));

  return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
                          nl->OneElemList(nl->StringAtom(relName)),
                          streamType);
}

/*
5.14.2 Value mapping

*/
class FFeedLocalInfo: public ProgressLocalInfo
{
public:
  FFeedLocalInfo(string filePath, ListExpr streamTypeList)
  : tupleBlockFile(0)
  {
    if (!FileSystem::FileOrFolderExists(filePath))
    {
        cerr << "Error: File '" << filePath
            << "' doesn't exist!\n" << endl;
    }
    else
    {
      tupleBlockFile = new ifstream(filePath.c_str(), ios::binary);
      if (!tupleBlockFile->good())
      {
        cerr << "Error accessing file '" << filePath << "'\n\n";
        tupleBlockFile = 0;
      }
    }
    tupleType = new TupleType(SecondoSystem::GetCatalog()
                    ->NumericType(nl->Second(streamTypeList)));

    if (tupleBlockFile)
    {
      //get the description list
      u_int32_t descSize;
      size_t fileLength;
      tupleBlockFile->seekg(0, ios::end);
      fileLength = tupleBlockFile->tellg();
      tupleBlockFile->seekg(
          (fileLength - sizeof(descSize)), ios::beg);
      tupleBlockFile->read((char*)&descSize, sizeof(descSize));

      char descStr[descSize];
      tupleBlockFile->seekg(
          (fileLength - (descSize + sizeof(descSize))), ios::beg);
      tupleBlockFile->read(descStr, descSize);
      tupleBlockFile->seekg(0, ios::beg);

      NList descList = NList(binDecode(string(descStr)));

      //Initialize the sizes of progress local info
      noAttrs = tupleType->GetNoAttributes();
      total = descList.first().intval();
      attrSize = new double[noAttrs];
      attrSizeExt = new double[noAttrs];
      for(int i = 0; i < noAttrs; i++)
      {
        attrSizeExt[i] =
            descList.elem(4 + i*2).realval() / total;
        attrSize[i] =
            descList.elem(4 + (i*2 + 1)).realval() / total;

        SizeExt += attrSizeExt[i]; //average sizeExt of a tuple
        Size += attrSize[i];
      }

      sizesInitialized = true;
      sizesChanged = true;
    }
  }
  ~FFeedLocalInfo() {
    if (tupleBlockFile)
    {
      delete tupleBlockFile;
      tupleBlockFile = 0;
    }
    if (tupleType)
    {
      tupleType->DeleteIfAllowed();
    }
  }

  Tuple* getNextTuple(){
    if (0 == tupleBlockFile )
      return 0;

    Tuple* t = 0;
    u_int32_t blockSize;
    tupleBlockFile->read(
        reinterpret_cast<char*>(&blockSize),
        sizeof(blockSize));
    if (!tupleBlockFile->eof() && (blockSize > 0))
    {
      blockSize -= sizeof(blockSize);
      char tupleBlock[blockSize];
      tupleBlockFile->read(tupleBlock, blockSize);
      t = new Tuple(tupleType);
      t->ReadFromBin(tupleBlock, blockSize);
    }

    return t;
  }

  ifstream *tupleBlockFile;
  TupleType* tupleType;

};

int FFeedValueMap(Word* args, Word& result,
    int message, Word& local, Supplier s)
{
  int index = -1;
  string relName, path;
  FFeedLocalInfo* ffli = 0;
  Supplier sonOfFeed;

  switch(message)
  {
    case OPEN: {
      path = ((CcString*)args[1].addr)->GetValue();
      if (qp->GetNoSons(s) == 4)
      {
        index = ((CcInt*)args[2].addr)->GetIntval();
        relName = ((CcString*)args[3].addr)->GetValue();
      }
      else
        relName = ((CcString*)args[2].addr)->GetValue();

      if (path == "")
      {
        path = FileSystem::GetCurrentFolder();
        FileSystem::AppendItem(path, "cell");
        FileSystem::AppendItem(path, relName);
      }

      if(index > 0)
      {
        stringstream ss;
        ss << path << "_" << index;
        path = ss.str();
      }

      ffli = (FFeedLocalInfo*) local.addr;
      if (ffli) delete ffli;
      ffli = new FFeedLocalInfo(path, qp->GetType(s));

      ffli->returned = 0;
      local.setAddr(ffli);
      return 0;
    }
    case REQUEST: {
      ffli = (FFeedLocalInfo*)local.addr;
      Tuple *t = ffli->getNextTuple();
      if (0 == t)
        return CANCEL;
      else
      {
        ffli->returned++;
        result.setAddr(t);
        return YIELD;
      }

    }
    case CLOSE: {
      ffli = (FFeedLocalInfo*)local.addr;
      if (ffli->tupleBlockFile){
        ffli->tupleBlockFile->close();
        delete ffli->tupleBlockFile;
        ffli->tupleBlockFile = 0;
      }
    }
    case CLOSEPROGRESS: {
      sonOfFeed = qp->GetSupplierSon(s, 0);
      ffli = (FFeedLocalInfo*) local.addr;
      if ( ffli )
      {
         delete ffli;
         local.setAddr(0);
      }
      return 0;
    }
    case REQUESTPROGRESS: {

      ProgressInfo p1;
      ProgressInfo *pRes = 0;
      const double uFeed = 0.00194;    //milliseconds per tuple
      const double vFeed = 0.0000196;  //milliseconds per Byte

      pRes = (ProgressInfo*) result.addr;
      ffli = (FFeedLocalInfo*) local.addr;

      if (ffli)
      {
        ffli->sizesChanged = false;

/*
This operator should always be the first operator of a tuple,
therefore it doesn't have any son operator.

*/
        pRes->Card = (double)ffli->total;
        pRes->CopySizes(ffli);
        pRes->Time =
            (ffli->total + 1) * (uFeed + ffli->SizeExt * vFeed);
        pRes->Progress =
            ffli->returned * (uFeed + ffli->SizeExt * vFeed)
            / pRes->Time;
        pRes->BTime = 0.001;
        pRes->BProgress = 1.0;

        return YIELD;
      }
      else
        return CANCEL;
    }
  }
  return 0;
}

const string FFeedSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" ) "
  "( <text>(relName x string x [int] ) -> "
  "( stream(tuple(...)) )</text--->"
  "<text>ffeed( _, _, _ )</text--->"
  "<text>restore a relation from a binary file "
  "created by ~fconsume~ operator.</text--->"
  ") )";

Operator ffeedOp (
    "ffeed",               // name
    FFeedSpec,             // specification
    FFeedValueMap,                 // value mapping
    Operator::SimpleSelect, // trivial selection function
    FFeedTypeMap           // type mapping
);

/*
3 Class ~HadoopParallelAlgebra~

A new subclass ~HadoopParallelAlgebra~ of class ~Algebra~ is declared.
The only specialization with respect to class ~Algebra~ takes place within
the constructor: all type constructors and operators are registered at the
actual algebra.

After declaring the new class, its only instance ~extendedRelationAlgebra~
is defined.

*/

class HadoopParallelAlgebra: public Algebra
{
public:
  HadoopParallelAlgebra() :
    Algebra()
  {

    AddOperator(doubleExportInfo(),
        doubleExportValueMap, doubleExportTypeMap);
    AddOperator(paraHashJoinInfo(),
        paraHashJoinValueMap, paraHashJoinTypeMap);

    AddOperator(paraJoinInfo(),
        paraJoinValueMap, paraJoinTypeMap);

    AddOperator(add0TupleInfo(),
        add0TupleValueMap, add0TupleTypeMap);


    AddOperator(TUPSTREAMInfo(), 0, TUPSTREAMType);
    AddOperator(TUPSTREAM2Info(), 0, TUPSTREAM2Type);
    AddOperator(TUPSTREAM3Info(), 0, TUPSTREAM3Type);

    AddOperator(&fconsumeOp);
    AddOperator(&ffeedOp);

#ifdef USE_PROGRESS
    fconsumeOp.EnableProgress();
    ffeedOp.EnableProgress();
#endif

  }
  ~HadoopParallelAlgebra()
  {
  }
  ;
};

/*
4 Initialization

Each algebra module needs an initialization function. The algebra manager
has a reference to this function if this algebra is included in the list
of required algebras, thus forcing the linker to include this module.

The algebra manager invokes this function to get a reference to the instance
of the algebra class and to provide references to the global nested list
container (used to store constructor, type, operator and object information)
and to the query processor.

The function has a C interface to make it possible to load the algebra
dynamically at runtime.

*/

extern "C" Algebra*
InitializeHadoopParallelAlgebra(
    NestedList* nlRef, QueryProcessor* qpRef)
{
  nl = nlRef;
  qp = qpRef;
  return (new HadoopParallelAlgebra());
}

/*
[newpage]

*/

