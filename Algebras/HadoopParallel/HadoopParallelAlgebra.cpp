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

#include "HadoopParallelAlgebra.h"
#include <vector>
#include <iostream>
#include <string>
#include "RelationAlgebra.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "Counter.h"
#include "TupleIdentifier.h"
#include "LogMsg.h"
#include "ListUtils.h"
#include "FTextAlgebra.h"
#include "Symbols.h"
#include "Base64.h"
#include "regex.h"
#include "FileSystem.h"
#include "StringUtils.h"
#include "Symbols.h"

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
        "stream(tuple(a1 ... ai ... an)) "
        "x stream(tuple(b1 ... bj ... bm)) "
        "x ai x bj -> stream(tuple"
        "(key:string)(value:string))";
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
              nl->SymbolAtom(CcString::BasicType())),
          nl->TwoElemList(nl->StringAtom("valueT",false),
              nl->SymbolAtom(FText::BasicType())));
      NList AttrList(attrList, nl);
      NList tupleStreamList =
          NList(NList().tupleStreamOf(AttrList));

      return nl->ThreeElemList(
                 nl->SymbolAtom(Symbol::APPEND()),
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
    newTuple->PutAttribute(0,new CcString(true, key));
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
  valueStr = stringutils::replaceAll(valueStr, "\n", "");
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
        "-> stream(tuple((a1 t1) ... "
        "(an tn)(b1 p1) ... (bm pm)))";
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
        nl->Second(nl->First(streamTupleList)),FText::BasicType()))
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
    ListExpr resultList = nl->TwoElemList(
        nl->SymbolAtom(Symbol::STREAM()),
        nl->TwoElemList(
            nl->SymbolAtom(Tuple::BasicType()),
            resultAttrList));

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
        "from the first argument, "
        "and forward it as a stream";
  }
};

ListExpr TUPSTREAMType( ListExpr args)
{

  if (nl->ListLength(args) < 1)
    return listutils::typeError("Expect one argument at least");
  ListExpr first = nl->First(args);
  if (!listutils::isRelDescription(first))
    return listutils::typeError("rel(tuple(...)) expected");
  return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                         nl->Second(first));
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
  if (nl->ListLength(args) < 2)
    return listutils::typeError("Expect two argument at least");
  ListExpr second = nl->Second(args);
  if (!listutils::isRelDescription(second))
    return listutils::typeError("rel(tuple(...)) expected");
  return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                         nl->Second(second));
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
        "from the third argument, "
        "and forward it as a stream";
  }
};

ListExpr TUPSTREAM3Type( ListExpr args)
{
  if (nl->ListLength(args) < 3)
    return listutils::typeError("Expect 3 arguments at least");
  ListExpr third = nl->Third(args);
  if (!listutils::isRelDescription(third))
      return listutils::typeError("rel(tuple(...)) expected");
  return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                         nl->Second(third));
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
        "x(map (stream(T1)) (stream(T2)) "
        "(stream(T1 T2))) ) -> stream(tuple(T1 T2))";
    syntax = "_ _ _ parajoin [fun]";
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
  if (nl->ListLength(args) == 4)
  {
    // parajoin for taking mixed streams

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
        nl->Second(nl->First(attrList)),FText::BasicType()))
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
                nl->SymbolAtom(Symbol::STREAM()),
                nl->TwoElemList(nl->SymbolAtom(Tuple::BasicType()),
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
  else
  {
    ErrorReporter::ReportError(
      "Operator parajoin expect a list of four arguments");
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
6 Parajoin2

This is a modified version of *parajoin* operator.
The main difference of the new operator is that,
it accepts two separated sorted tuple stream,
and collect tuples have a same key attribute value,
then use the parameter join function to process them.

*/
struct paraJoin2Info : OperatorInfo
{
  paraJoin2Info()
  {
    name = "parajoin2";
    signature =
        "( (stream(tuple(T1))) x (stream(tuple(T2)))"
        "x (map (stream(T1)) (stream(T2)) "
        "(stream(T1 T2))) ) -> stream(tuple(T1 T2))";
    syntax = "_ _ parajoin2 [ _, _ ; fun]";
    meaning = "use parameter join function to merge join two "
              "input sorted streams according to key values.";
  }
};


/*
Take another two sorted stream,
then use the parameter function to execute merge-join operation.

----
   ( (stream(tuple((a1 t1) (a2 t2) ... (ai ti) ... (am tm) )))
   x (stream(tuple((b1 p1) (b2 p2) ... (bj tj) ... (bn pn) )))
   x ai x bj
   x ((map (stream((a1 t1) (a2 t2) ... (am tm) ))
           (stream((b1 p1) (b2 p2) ... (bn pn) ))
           (stream((a1 t1) (a2 t2) ... (am tm)
                   (b1 p1) (b2 p2) ... (bn pn)))))  )
   -> stream(tuple((a1 t1) (a2 t2) ... (am tm)
                   (b1 p1) (b2 p2) ... (bn pn)))
----

*/

ListExpr paraJoin2TypeMap(ListExpr args)
{
  if(nl->ListLength(args) == 5)
    {
      NList l(args);
      NList streamA = l.first();
      NList streamB = l.second();
      NList keyA = l.third();
      NList keyB = l.fourth();
      NList mapList = l.fifth();

      string err = "parajoin2 expects "
          "(stream(tuple(T1)) x stream(tuple(T2)) "
          "x string x string "
          "x (map (stream(T1)) (stream(T2)) (stream(T1 T2))))";
      string err1 = "parajoin2 can't found key attribute : ";

      NList attrA;
      if (!streamA.checkStreamTuple(attrA))
        return l.typeError(err);

      NList attrB;
      if (!streamB.checkStreamTuple(attrB))
        return l.typeError(err);

      ListExpr keyAType, keyBType;
      int keyAIndex = listutils::findAttribute(
                                 attrA.listExpr(),
                                 keyA.convertToString(),
                                 keyAType);
      if ( keyAIndex <= 0 )
        return l.typeError(err1 + keyA.convertToString());

      int keyBIndex = listutils::findAttribute(
                                 attrB.listExpr(),
                                 keyB.convertToString(),
                                 keyBType);
      if ( keyBIndex <= 0 )
        return l.typeError(err1 + keyB.convertToString());

      if (!nl->Equal(keyAType, keyBType))
        return l.typeError(
            "parajoin2 expects two key attributes with same type.");

      NList attrResult;
      if (mapList.first().isSymbol(Symbol::MAP())
          && mapList.second().first().isSymbol(Symbol::STREAM())
          && mapList.second().
             second().first().isSymbol(Tuple::BasicType())
          && mapList.third().first().isSymbol(Symbol::STREAM())
          && mapList.third().
             second().first().isSymbol(Tuple::BasicType())
          && mapList.fourth().checkStreamTuple(attrResult)  )
      {
        NList resultStream =
            NList(NList(Symbol::STREAM(),
                        NList(NList(Tuple::BasicType()),
                              attrResult)));

        return NList(NList(Symbol::APPEND()),
                     NList(NList(keyAIndex), NList(keyBIndex)),
                     resultStream).listExpr();
      }
      else
        return l.typeError(err);
    }
    else
    {
      ErrorReporter::ReportError(
        "Operator parajoin expect a list of five arguments");
      return nl->TypeError();
    }
}

int paraJoin2ValueMap(Word* args, Word& result,
                int message, Word& local, Supplier s)
{
  pj2LocalInfo* li=0;

  switch(message)
  {
    case OPEN:{
      qp->Open(args[0].addr);
      qp->Open(args[1].addr);

      if (li)
        delete li;
      li = new pj2LocalInfo(args[0], args[1],
                            args[5], args[6],
                            args[4], s);
      local.setAddr(li);
      return 0;
    }
    case REQUEST:{
      if (0 == local.addr)
        return CANCEL;
      li = (pj2LocalInfo*)local.addr;

      result.setAddr(li->getNextTuple());
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
      if (0 == local.addr)
        return CANCEL;
      li = (pj2LocalInfo*)local.addr;

      result.setAddr(li->getNextInputTuple(tupBufferA));
      if (result.addr)
        return YIELD;
      else
        return CANCEL;
    }
    case (2*FUNMSG)+REQUEST:{
      if (0 == local.addr)
        return CANCEL;
      li = (pj2LocalInfo*)local.addr;

      result.setAddr(li->getNextInputTuple(tupBufferB));
      if (result.addr)
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
      if (0 == local.addr)
        return CANCEL;
      li = (pj2LocalInfo*)local.addr;

      delete li;
      qp->Close(args[0].addr);
      qp->Close(args[1].addr);
      return 0;
    }
  }

  //should never be here
  return 0;
}

bool pj2LocalInfo::LoadTuples()
{
  bool loaded = false;

  //Clear the buffer
  if (ita)
    delete ita; ita = 0;
  if (tba)
    delete tba; tba = 0;

  if (itb)
    delete itb; itb = 0;
  if (tbb)
    delete tbb; tbb = 0;

  if (moreInputTuples)
  {
    if (cta == 0){
      cta.setTuple(NextTuple(streamA));
    }
    if (ctb == 0){
      ctb = NextTuple(streamB);
    }
  }
  if ( cta == 0 || ctb == 0)
  {
    //one of the stream is exhausted
    endOfStream = true;
    moreInputTuples = false;
    return loaded;
  }

  int cmp = CompareTuples(cta.tuple, keyAIndex,
                          ctb.tuple, keyBIndex);

  // Assume both streams are ordered by asc
  while(0 != cmp)
  {
    if (cmp < 0)
    {
      //a < b, get more a until a >= b
      while (cmp < 0)
      {
        cta.setTuple(NextTuple(streamA));
        if ( cta == 0 )
        {
          endOfStream = true;
          return loaded;
        }
        cmp = CompareTuples(cta.tuple, keyAIndex,
                            ctb.tuple, keyBIndex);
      }
    }
    else if (cmp > 0)
    {
      //a > b, get more b until a <= b
      while (cmp > 0)
      {
        ctb = NextTuple(streamB);
        if ( ctb == 0 )
          {
            endOfStream = true;
            return loaded;
          }
        cmp = CompareTuples(cta.tuple, keyAIndex,
                            ctb.tuple, keyBIndex);
      }
    }
  }

  //Take all tuples from streamA, until the next tuple is bigger
  //than the current one.
  tba = new TupleBuffer(maxMem);
  int cmpa = 0;
  RTuple lta;
  while ( (cta != 0) && (0 == cmpa) )
  {
    lta = cta;
    tba->AppendTuple(lta.tuple);
    cta.setTuple(NextTuple(streamA));
    if ( cta != 0 )
      cmpa = CompareTuples(lta.tuple, keyAIndex,
                           cta.tuple, keyAIndex);
  }

  tbb = new TupleBuffer(maxMem);
  int cmpb = 0;
  RTuple ltb;
  while ( (ctb != 0) && (0 == cmpb) )
  {
    ltb = ctb;
    tbb->AppendTuple(ltb.tuple);
    ctb.setTuple(NextTuple(streamB));
    if ( ctb != 0 )
      cmpb = CompareTuples(ltb.tuple, keyBIndex,
                           ctb.tuple, keyBIndex);
  }
  if ((cta == 0) || (ctb == 0)){
    moreInputTuples = false;
  }

  if ((0 == tba->GetNoTuples()) || (0 == tbb->GetNoTuples()))
  {
    endOfStream = true;
    return loaded;
  }

  ita = tba->MakeScan();
  itb = tbb->MakeScan();
  loaded = true;

  return loaded;
}

int pj2LocalInfo::CompareTuples(Tuple* ta, int kai,
                                Tuple* tb, int kbi)
{
  Attribute* a = static_cast<Attribute*>(ta->GetAttribute(kai));
  Attribute* b = static_cast<Attribute*>(tb->GetAttribute(kbi));

  if (!a->IsDefined() || !b->IsDefined()){
    cerr << "Undefined Tuples are contained." << endl;
    return -1;
  }

  int cmp = a->Compare(b);
  return cmp;
}

Tuple* pj2LocalInfo::getNextTuple()
{
  Word funResult(Address(0));

  while(!endOfStream)
  {
    qp->Request(pf, funResult);
    if (funResult.addr)
      return (Tuple*)funResult.addr;
    else if (endOfStream)
    {
      qp->Close(pf);
      return 0;
    }
    else
    {
      qp->Close(pf);
      if (LoadTuples())
        qp->Open(pf);
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
        "((stream(tuple((keyT string)(valueT text))))"
        "-> stream(tuple((keyT string)(valueT text))) )";
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
  && listutils::isSymbol(
      nl->Second(nl->First(tupleList)), CcString::BasicType())
  && listutils::isSymbol(nl->Second(
      nl->Second(tupleList)), FText::BasicType()))
  {
    return streamNL;
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

//Set up the remote copy command options uniformly.
const string scpCommand = "scp -o Connecttimeout=5 ";



/*
7 Implementation of clusterInfo class

The clusterInfo class is used to read two line-based text files that
describe the distribution of a cluster.
The locations of these files are denoted by two environment
variables: PARALLEL\_SECONDO\_MASTER and PARALLEL\_SECONDO\_SLAVES.
The first one is used to describe the master node of the cluster,
and can only contains one line.
The second one lists all possible locations within the cluster that
can hold type and data files, which can be written and read by
fconsume and ffeed operators respectively.

Each line of the files are composed by three parts, which are separated
by colons:

----
IP:location:port
----

The IP indicates the network position of a node inside the cluster,
and the location which must be a absolute path, describes the disk
position of the files.
The port is used to tell through which port to access to the Secondo
monitor that is allocated with this file location.

Each Secondo monitor reads its configurations from a file
that is denoted by SECONDO\_CONFIG, which also describes above three
parameters. And all functions inside this class of getting local
information like ~getLocalIP~ reads the configuration from this file.
In principle, the information of the config file should be
in conformity with the list files, or else operators like ~fconsume~
may goes wrong.

*/
clusterInfo::clusterInfo(bool _im) :
    ps_master("PARALLEL_SECONDO_MASTER"),
    ps_slaves("PARALLEL_SECONDO_SLAVES"),
    isMaster(_im), localNode(-1)
{
  if (isMaster)
    fileName = string(getenv(ps_master.c_str()));
  else
    fileName = string(getenv(ps_slaves.c_str()));

  ok = false;
  if (fileName.length() == 0)
    cerr << "Environment variable "
         << (isMaster ? ps_master : ps_slaves)
         << " is not defined." << endl;
  if (!FileSystem::FileOrFolderExists(fileName))
    cerr << "File (" << fileName << ") is not exist." << endl;
  if (FileSystem::IsDirectory(fileName))
    cerr << fileName << " is a directory" << endl;

  ifstream fin(fileName.c_str());
  string line;
  disks = new vector<pair<string, pair<string, int> > >();
  while (getline(fin, line))
  {
    if (line.length() == 0)
      continue;  //Avoid warning message for an empty line
    istringstream iss(line);
    string ipAddr, cfPath, sport;
    getline(iss, ipAddr, ':');
    getline(iss, cfPath, ':');
    getline(iss, sport, ':');
    if ((ipAddr.length() == 0) ||
        (cfPath.length() == 0) ||
        (sport.length() == 0))
    {
      cerr << "Format in file " << fileName << " is not correct.\n";
      break;
    }

    //Remove the slash tail
    if (cfPath.find_last_of("/") == cfPath.length() - 1)
      cfPath = cfPath.substr(0, cfPath.length() - 1);

    int port = atoi(sport.c_str());
    disks->push_back(pair<string, pair<string, int> >(
        ipAddr, pair<string, int>(cfPath, port)));
  }
  fin.close();
  if (disks->size() > 0)
  {
    ok = true;
    if (isMaster && (disks->size() > 1))
      cerr << "Master list can only have one line" << endl;
  }
}

/*
The local IP address can be set inside the SecondoConfig file,
but if the setting value doesn't match with any available IP
addresses of the current machine, then an error message will be given.

If it's not defined, then we use all available IP addresses
to match with the slave list.
If nothing is matched, then an error message will be given.

If the error message is given, then the return an empty string.

*/
string clusterInfo::getLocalIP()
{
  string localIP;

  string confPath = string(getenv("SECONDO_CONFIG"));
  localIP = SmiProfile::GetParameter("ParallelSecondo",
      "localIP","", confPath);

  bool match = false;
  vector<string> *aIPs = getAvailableIPAddrs();
  for (vector<string>::iterator it = aIPs->begin();
       it != aIPs->end(); it++)
  {
    string aIP = (*it);
    if (localIP != "")
    {
      if (localIP.compare(aIP) == 0)
        match = true;
    }
    else
    {
      for(vector<diskDesc>::iterator dit = disks->begin();
          dit != disks->end(); dit++)
      {
        if (dit->first.compare(aIP) == 0)
        {
          localIP = aIP;
          match = true;
        }
      }
    }

    if (match) break;
  }

  if (!match)
    cerr << "Host's IP address is "
        "undefined in PARALLEL_SLAVES list. \n" << endl;

  return localIP;
}

vector<string>* clusterInfo::getAvailableIPAddrs()
{
  vector<string>* IPList = new vector<string>();
  struct ifaddrs * ifAddrStruct = 0;
  struct ifaddrs * ifa = 0;
  void * tmpAddrPtr = 0;

  getifaddrs(&ifAddrStruct);
  for (ifa = ifAddrStruct; ifa != 0; ifa = ifa->ifa_next)
  {
    if (ifa->ifa_addr->sa_family == AF_INET)
    {
      // IPv4 Address
      tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
      char addressBuffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, tmpAddrPtr,
          addressBuffer, INET_ADDRSTRLEN);
      IPList->push_back(addressBuffer);
    }
    else if (ifa->ifa_addr->sa_family == AF_INET6)
    {
      // IPv6 Address
      tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
      char addressBuffer[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, tmpAddrPtr,
          addressBuffer, INET6_ADDRSTRLEN);
      IPList->push_back(addressBuffer);
    }
  }

  if (ifAddrStruct)
    freeifaddrs(ifAddrStruct);

  return IPList;
}

/*
Get the remote file path, make sure the remote is accessible.
All files are divided into sub-folders according to their prefix names.
If the sub-folder is not exit, then create it

*/

string clusterInfo::getRemotePath(size_t loc, string filePrefix,
    bool round, bool createPath, bool attachIP,
    bool attachProducerIP, string producerIP)
{
  string remotePath = "";

  if (!round)
  {
    assert(loc <= disks->size());
    loc -= 1;
  }
  else
    loc = (loc - 1) % disks->size();

  string IPAddr = (*disks)[loc].first;
  string rfPath = (*disks)[loc].second.first;
  if (attachProducerIP)
  {
    if (producerIP.length() == 0)
      producerIP = getLocalIP();
    filePrefix += ("_" + producerIP);
  }

  remotePath = (attachIP ? (IPAddr + ":") : "") + rfPath;
  return remotePath;
}



string clusterInfo::getIP(size_t loc, bool round)
{
  if (!round)
  {
    assert(loc <= disks->size());
    loc -= 1;
  }
  else
    loc = (loc - 1) % disks->size();

  return (*disks)[loc].first;
}

int clusterInfo::searchLocalNode()
{
  string localPath = getLocalPath();
  string localIP = getLocalIP();

  int local = -1, cnt = 1;
  if (localIP.length() != 0 &&
      localPath.length() != 0)
  {
    vector<diskDesc>::iterator iter;
    for (iter = disks->begin(); iter != disks->end(); iter++, cnt++)
    {
      if ((localIP.compare((*iter).first) == 0) &&
          (localPath.compare((*iter).second.first)) == 0)
      {
        local = cnt;
        break;
      }
    }
  }
  else
    cerr << "\nThe local IP or Path is not correctly defined. "
        "They should match one line in PARALLEL_SECONDO_SLAVES list"
        "Check the SECONDO_CONFIG file please." << endl;

  return local;
}

void clusterInfo::print()
{
  if (ok)
  {
    int counter = 1;
    cout << "\n---- PARALLEL_SECONDO_SLAVES list ----" << endl;
    vector<diskDesc>::iterator iter;
    for (iter = disks->begin(); iter != disks->end();
        iter++, counter++)
    {
      cout << counter << ":\t" << iter->first
          << "\t" << iter->second.first
          << "\t" << iter->second.second << endl;
    }
    cout << "---- PARALLEL_SECONDO_SLAVES ends ----\n" << endl;
  }
}
/*
5 Operator ~fconsume~

This operator maps

----   ( stream(tuple(...)) x fileName x path x [fileIndex]
             x [typeNode1] x [typeNode2]
             x [array(string) x selfIndex
                x targetIndex x duplicateTimes] )-> bool
----

Operator ~fconsume~ exports the accepted tuple-stream into files.
The tuples are written into a binary file,
and the type list is written into a separate text file.
Totally it has three different modes:

  * Local mode
  * Type remote mode
  * Data remote mode

Local mode means ~fconsume~ writes both the binary file
and the type file to current node.
The type remote mode means besides writing both files to local disk,
the operator copies the type file to at most two specified remote nodes.
At last the data remote mode means both the type file
and the tuple file are copied into remote nodes,
and if required, delete the tuple file from the local node.

This operator supports at most 10 arguments, the top three are necessary,
then the next three are optional. And the end four arguments are
optional as a whole, i.e., if required, these four arguments must be
asked as a whole.

The first three necessary arguments are:

  * tuple stream
  * file name
  * file path

The file name must not be empty. If the file name is given as "FILE",
then the exported type file's name is FILE\_type.
And the binary tuple file's name is FILE.
The file path could be empty, and then the files are put into
the default path \$SECONDO\_BUILD\_DIR/bin/parallel/.
If it is not empty, the given path must be an absolute Unix path.

The next three optional arguments are:

  * file index
  * type node1
  * type node2

The fourth argument ~file index~ is optional,
it gives an identifiable postfix to the binary file.
If it's given, then the binary tuple file's name is FILE\_index.
The fifth and the sixth arguments denote two remote nodes' names.
If one of them is set, then the operator is changed to type remote mode. .

Note for transporting files between different machines,
we use utility ~scp~ to copy files, and the passwords of these nodes
should not be asked while coping files.

The last four arguments are:

  * machine array
  * self index
  * target index
  * duplicate times

These four arguments should be asked as a whole, and if they are set,
then the operator is changed to data remote mode.
The machine array is an array of strings,
each element is the name of a remote node which
keeps a non-password-required ssh connection with the current node.
The self index, is the local node's index inside the machine array.
The target index, is the array index of the first target node
to which the operator duplicates the binary file.
The last argument duplicate times indicates how many remote
nodes the binary file is copied to.
If it is bigger than 1, then the operator will not only copies the
binary file to the machine where the target index point to,
but also copies the file to next (~duplicate times~ - 1) remote machines.
If the nodes specified by ~target index~ and ~duplicate times~
don't contain the local node,
then the produced binary file will be removed after the replication.

5.1 Update the format of fconsume

In 14/05/2011, remove the machine array parameter of in data remote mode,
as building a Secondo array object that describes the whole structure
of the cluster in every database, limits the flexibility of the whole system.
Therefore, we use a text file list that is denoted by \$PARALLEL\_SECONDO\_SLAVES
to take the place of the machine array.

In both type remote and data remote modes, target nodes that are
used to backup type file and data files must be registered
in the node list file specified by \$PARALLEL\_SECONDO\_SLAVES.

Now the operator maps

---- (stream(tuple(...))
      x fileName x filePath x [fileSuffix]
      x [typeLoc1] x [typeLoc2]
      x [targetLoc x dupTimes])
     -> bool
----

Besides the input tuple stream, all the left parameters are divided
into three parts, separated by semicolons, and correspond to the
three modes of the operator.

The basic functions of the operator and its different modes don't
change, only the locations of remote type nodes and remote data nodes
are not given by users explicitly, but are denoted by giving
serial numbers of the \$PARALLEL\_SECONDO\_SLAVES.
The format of the list file is described in the ~clusterInfo~ section.

Besides, during the data remote mode, the operator should knows
the serial number of the current location before duplicating files,
which requires the operator to get to know the current IP address.
However, I didn't a suitable method to get the local IP address
in different platforms, therefore this IP address must be set inside
the configure file denoted by \$SECONDO\_CONFIG, as localIP value.

The location of the files is also set up inside the configure file,
as SecondoFilePath, in case the \$PARALLEL\_SECONDO\_SLAVES is not
required within an individual computer.

In 8/6/2011, increase another parameter into the fconsume operator, rowNum.
As ~ffeed~, we put it in the front of the fileSuffix paramter.
However, we don't strictly distinguish these two parameters.
If both of them is available, then we set the data files' names
with two successive integers connected by a underscore.
If only one number shows up, then only one integer suffix is set
after data files' names.

Now the operator maps

---- (stream(tuple(...))
      x fileName x filePath x [rowNum] x [fileSuffix] ;
      x [typeLoc1] x [typeLoc2]                       ;
      x [targetLoc x dupTimes])                       ;
     -> bool
----


5.2 Specification

*/

struct FConsumeInfo : OperatorInfo {
  FConsumeInfo() : OperatorInfo()
  {
    name =      "fconsume";
    signature = "stream(tuple( ... )) "
        "x string x text x [int] "
        "x [ [int] x [int] ] "
        "x [ int x int ] "
        "-> bool";
    syntax  = "stream(tuple( ... )) "
        "fconsume[ fileName, filePath, [rowNum], [fileSuffix]; "
        "[typeNode1] x [typeNode2]; "
        "[targetIndex x dupTimes] ] ";
    meaning =
        "Export a stream of tuples' data into a binary data file, "
        "and its type nested list into a text type file. "
        "The given file name is used as the data file's name. "
        "If the optional integer value fileSuffix is given, "
        "then the data file's name will be 'fileName_fileSuffix'."
        "The type file name is 'fileName_type'. "
        "Both type file and data file can be duplicated "
        "to some remote machines, which are listed in a list file "
        "indicated by PARALLEL_SECONDO_SLAVES environment variable. "
        "Detail explanations are described in the attached "
        "README.pdf along with the HadoopParallel algebra.";
  }
};

/*
5.3 Type mapping

*/
ListExpr FConsumeTypeMap(ListExpr args)
{
  NList l(args);
  string lengthErr =
      "ERROR!Operator fconsume expects 4 parameter groups, "
      "separated by semicolons";
  string typeErr = "ERROR!Operator fconsume expects "
               "(stream(tuple(...)) "
               "fileName: string, filePath: text, "
               "[rowNum: int] x [fileSuffix: int]; "
               "[typeNodeIndex: int] [typeNodeIndex2: int]; "
               "[targetNodeIndex: int, duplicateTimes: int])";
  string typeErr2 =
      "ERROR!The basic parameters expects "
      "[fileName: string, filePath: text, "
      "[rowNum: int], [fileSuffix: int]]";
  string typeErr3 = "ERROR!Type remote nodes expects "
      "[[typeNodeIndex: int], [typeNodeIndex2: int]]";
  string typeErr4 = "ERROR!Data remote nodes expects "
      "[targetNode:int, duplicateTimes: int]";
  string err1 = "ERROR!The file name should NOT be empty!";
  string err2 = "ERROR!Cannot create type file: \n";
  string err3 = "ERROR!Infeasible evaluation in TM for attribute: ";
  string err4 = "ERROR!Expect the file name and path.";
  string err5 = "ERROR!Expect the file suffix.";
  string err6 = "ERROR!Expect the target index and dupTimes.";
  string err7 = "ERROR!Remote node for type file is out of range";
  string err8 = "ERROR!The slave list file does not exist."
      "Is $PARALLEL_SECONDO_SLAVES correctly set up ?";
  string err9 = "ERROR!Remote copy type file fail.";

  int len = l.length();
  if ( len != 4)
    return l.typeError(lengthErr);

  string filePrefix, filePath;
  bool trMode, drMode;
  drMode = trMode = false;
  int tNode[2] = {-1, -1};

  NList tsList = l.first(); //input tuple stream
  NList bsList = l.second(); //basic parameters
  NList trList = l.third();  //type remote parameters
  NList drList = l.fourth(); //data remote parameters

  NList attr;
  if(!tsList.first().checkStreamTuple(attr) )
    return l.typeError(typeErr);

  //Basic parameters
  //The first list contains all parameters' types
  NList pType = bsList.first();
  //The second list contains all parameter's values
  NList pValue = bsList.second();
  if (pType.length() < 2 || pType.length() > 4)
    return l.typeError(typeErr2);

  if (pType.first().isSymbol(CcString::BasicType()) &&
      pType.second().isSymbol(FText::BasicType()))
  {
    if (pType.length() > 2)
    {
      if (!pType.third().isSymbol(CcInt::BasicType()))
        return l.typeError(err5);

      if ((4 == pType.length()) &&
          !pType.fourth().isSymbol(CcInt::BasicType()))
        return l.typeError(err5);
    }

    NList fnList;
    if (!QueryProcessor::GetNLArgValueInTM(pValue.first(), fnList))
      return l.typeError(err3 + "file prefix name");
    filePrefix = fnList.str();
    if (0 == filePrefix.length())
      return l.typeError(err1);
    NList fpList;
    if (!QueryProcessor::GetNLArgValueInTM(pValue.second(), fpList))
      return l.typeError(err3 + "filePath");
    filePath = fpList.str();
  }
  else
    return l.typeError(err4);

  pType = trList.first();
  if (!pType.isEmpty())
  {
    if (pType.length() > 2)
      return l.typeError(typeErr3);
    while (!pType.isEmpty())
    {
      if (!pType.first().isSymbol(CcInt::BasicType()))
        return l.typeError(typeErr3);
      pType.rest();
    }

    pValue = trList.second();
    trMode = true;
    int cnt = 0;
    while (!pValue.isEmpty())
    {
      NList nList;
      if (!QueryProcessor::GetNLArgValueInTM(pValue.first(), nList))
        return l.typeError(err3 + " type node index");
      tNode[cnt++] = nList.intval();
      pValue.rest();
    }
  }

  pType = drList.first();
  if (!pType.isEmpty())
  {
    if (pType.length() != 2)
      return l.typeError(err6);
    if (!pType.first().isSymbol(CcInt::BasicType()) ||
        !pType.second().isSymbol(CcInt::BasicType()))
      return l.typeError(typeErr4);
    drMode = true;
  }

  //Type Checking is done, create the type file.
  filePath = getLocalFilePath(filePath, filePrefix, "_type");
  if (filePath.length() == 0)
    return l.typeError(err2 +
        "Type file path is unavailable, check the SecondoConfig.ini.");
  ofstream typeFile(filePath.c_str());
  NList resultList = NList(NList(Relation::BasicType()),
                           tsList.first().second());
  if (typeFile.good())
  {
    typeFile << resultList.convertToString() << endl;
    typeFile.close();
    cerr << "Type file: " << filePath << " is created. " << endl;
  }
  else
    return l.typeError(
        err2 + "Type file path is unavailable: " + filePath);

  //Verify the existence of the PARALLEL\_SECONDO\_SLAVES file
  if (trMode || drMode)
  {
    clusterInfo *ci = new clusterInfo();
    if (!ci->isOK())
      return l.typeError(err8);
    int sLen = ci->getLines();
    //Copy type files to remote location
    for (int i = 0; i < 2; i++)
    {
      if (tNode[i] > 0)
      {
        if (tNode[i] > sLen)
        {
          ci->print();
          return l.typeError(err7);
        }
        string rPath = ci->getRemotePath(tNode[i], filePrefix);
        cerr << "Copy type file to -> \t" << rPath << endl;
        if ( 0 !=
            (system((scpCommand + filePath + " " + rPath).c_str())))
          return l.typeError(err9);
      }
    }
  }

  return NList(NList(CcBool::BasicType())).listExpr();
}


/*
5.4 Value mapping

*/
int FConsumeValueMap(Word* args, Word& result,
    int message, Word& local, Supplier s)
{
  fconsumeLocalInfo* fcli = 0;

  if ( message <= CLOSE)
  {
    result = qp->ResultStorage(s);
    Supplier bspList = args[1].addr,
             drpList = args[3].addr;
    string relName, fileSuffix = "", filePath;
    int fileIndex = -1;

    relName = ((CcString*)
      qp->Request(qp->GetSupplierSon(bspList, 0)).addr)->GetValue();
    filePath = ((FText*)
      qp->Request(qp->GetSupplierSon(bspList, 1)).addr)->GetValue();
    int bspLen = qp->GetNoSons(bspList);
    int idx = 2;
    while (idx < bspLen)
    {
      fileIndex = ((CcInt*)
        qp->Request(qp->GetSupplier(bspList, idx)).addr)->GetValue();
      if (fileIndex >= 0)
        fileSuffix += ("_" + int2string(fileIndex));
      idx++;
    }

    int ti = -1, dt = -1;
    bool drMode = false;

    int drpLen = qp->GetNoSons(drpList);
    if (drpLen == 2)
    {
      drMode = true;
      ti = ((CcInt*)
          qp->Request(qp->GetSupplier(drpList, 0)).addr)->GetValue();
      dt = ((CcInt*)
          qp->Request(qp->GetSupplier(drpList, 1)).addr)->GetValue();
    }

    //Check whether the duplicate parameters are available
    clusterInfo *ci = new clusterInfo();
    if (drMode && (ti > ci->getLines()))
    {
      ci->print();
      cerr <<
          "ERROR! The first target node for backing up duplicate "
          "data files is out of the range of the slave list.\n";
      ((CcBool*)(result.addr))->Set(true, false);
      return 0;
    }

    fcli = (fconsumeLocalInfo*) local.addr;
    if (fcli) delete fcli;

    fcli = new fconsumeLocalInfo();
    fcli->state = 0;
    fcli->current = 0;
    local.setAddr(fcli);

    //Write complete tuples into a binary file.
    //create a path for this file.
    filePath = getLocalFilePath(filePath, relName, fileSuffix);
    ofstream blockFile(filePath.c_str(), ios::binary);
    if (!blockFile.good())
    {
      cerr << "ERROR!Create file " << filePath << " fail!" << endl;
      ((CcBool*)(result.addr))->Set(true, false);
      return 0;
    }

    //Statistic information
    ListExpr relTypeList =
        qp->GetSupplierTypeExpr(qp->GetSon(s,0));
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
    cout << "\nData file: " << filePath << " is created" << endl;

    if (drMode)
    {
      bool keepLocal = false;
      int localNode = ci->getLocalNode();
      if (localNode < 1)
      {
        cerr
          << "ERROR! Cannot find the local position " << endl
          << ci->getLocalIP() << ":" << ci->getLocalPath() << endl
          << "in the slave list, backup files fail. " << endl;
        ci->print();

        ((CcBool*) (result.addr))->Set(true, false);
        return 0;
      }

      string cName = ci->getIP(localNode);
      string newFileName = relName + fileSuffix + "_" + cName;

      //Avoid copying file to a same node repeatedly
      int cLen = ci->getLines();
      bool copyList[cLen + 1];
      memset(copyList, false, (cLen + 1));
      for (int i = 0; i < dt; i++, ti++)
        copyList[((ti - 1)%cLen + 1)] = true;

      for (int i = 1; i <= cLen; i++)
      {
        if (copyList[i])
        {
          if (localNode == i)
          {
            keepLocal = true;
            continue;
          }
          else
          {
            string rPath = ci->getRemotePath(
                i, relName, true, true, true, true);
            FileSystem::AppendItem(rPath, (relName + fileSuffix));
            cerr << "Copy " << filePath << "\n->\t" << rPath << endl;
            if ( 0 != ( system(
                (scpCommand + filePath + " " + rPath).c_str())))
            {
              cerr << "Copy remote file fail." << endl;
              ((CcBool*)(result.addr))->Set(true, false);
              return 0;
            }
          }
        }
      }
      if (!keepLocal)
      {
        if ( 0 != (system(("rm " + filePath).c_str())))
        {
          cerr << "Delete local file " << filePath << " fail.\n";
          ((CcBool*)(result.addr))->Set(true, false);
          return 0;
        }
        cerr << "Local file '" + filePath + "' is deleted.\n";
      }
    }
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
    const double wConsume = 0.001338;//millisecs per byte in FLOB

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
          if ( p1.BTime < 0.1 && pipelinedProgress )
            //non-blocking,
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

Operator fconsumeOp(FConsumeInfo(),
    FConsumeValueMap, FConsumeTypeMap);

/*
6 Operator ~ffeed~

This operator maps

----
fileName x path x [fileIndex] x [typeNode]
x [ machineArray x targetIndex x attemptTimes]
-> stream(tuple(...))
----

Operator ~ffeed~ restore a tuple stream from files created
by ~fconsume~ operator.

The first two string arguments ~fileName~ and ~path~ are indispensable.
~fileName~ defines the name of the relation we want to read from,
and it should NOT be empty.
Argument ~path~ defines where the files are.
If it is empty, then the files are assumed in the default path
\$SECONDO\_BUILD\_DIR/. Or else it must be an absolute Unix path.

The third argument ~fileIndex~ is optional,
it defines a postfix of the binary tuple file.
Assume the ~fileName~ is FILE, if the ~fileIndex~ is not defined,
then the binary file's name is FILE,
or else the file's name is FILE\_fileIndex.

The fourth argument ~typeNode~ defines a remote node's
name which contains the type file of the relation.
It's also an optional argument, and if it's not defined,
then the type file must be put into the local default path
SECONDO\_BUILD\_DIR/bin/parallel/.
If it is defined, then the operator first use scp utility to copy
the type file from the remote node to the local default path.

Besides reading binary file from local hard disk,
~ffeed~ also support reading the file from a remote machine if these two
machines are linked by a non-password-required ssh connection.
If so, the following three arguments  ~machineArray~,
~targetIndex~ and ~attemptTimes~ must be given as a whole.

The ~machineArray~ is a Secondo array of strings, which contains the names
of the remote machines that the current node can access to by
non-password-required ssh.
The ~targetIndex~ is used to denote which node in ~machineArray~
contains the binary tuple file.
The ~attemptTimes~ is used when the ~ffeed~ can't copy the binary file
from the node which ~targetIndex~ point to, then it tries to read the
file from the next node (~targetIndex~ + 1),
until the following ~attemptTimes~ nodes are all tried.

6.1 Update the format of ffeed

In 18/05/2011, adjust ~ffeed~ operator to read the remote locations
from list files, not from the string array.

Now the operator maps

----
fileName x path x [fileIndex] x [typeNodeIndex]
x [targetNodeIndex x attemptTimes]
->stream(tuple(...))
----

Similar as the ~fconsume~ operator, parameters of this operator
are also divided into three parts, and are separated by semicolons.
Because of that, we change ~ffeed~ from a prefix operator to
a post operator, since the prefix operators don't accept semicolons.
The prefix parameter is the file name of string type.

The ~typeNodeIndex~ and ~targetNodeIndex~ denote the locations of
some specific nodes inside the cluster, which are listed
inside the \$PARALLEL\_SECONDO\_SLAVES file.

In 8/6/2011, add the row number into the ffeed operator.
The rowNumber doesn't affect the type file, only the data type.
If it's defined, then the ffeed will fetch a file with two
successive suffices. Now the operator maps

----
fileName
x path x [rowNum] x [fileIndex]     ;
x [typeNodeIndex]                   ;
x [targetNodeIndex x attemptTimes]  ;
->stream(tuple(...))
----


6.2 Specification

*/

struct FFeedInfo : OperatorInfo {

  FFeedInfo() : OperatorInfo()
  {
    name =      "ffeed";
    signature = "string x text x [int] x [int] x [int x int x int]"
        " -> stream(tuple(...))";
    syntax  = "fileName ffeed[ filePath, [fileSuffix]; "
        "[remoteTypeNode]; "
        "[producerIndex x targetIndex x attemptTimes] ]";
    meaning =
        "Restore a tuple stream from the binary data and "
        "text type files that are created by "
        "fconsume or fdistribute operator. "
        "Both type and data file can be fetched from "
        "remote machines which are listed in the "
        "PARALLEL_SECONDO_SLAVES file."
        "Detail explanations are described in the attached "
        "README.pdf along with the HadoopParallel algebra.";
  }
};

/*
6.3 Type mapping

*/

ListExpr FFeedTypeMap(ListExpr args)
{
  NList l(args);
  NList pType, pValue;
  bool haveIndex, trMode, drMode;
  haveIndex = trMode = drMode = false;

  string lenErr = "ERROR! Operator ffeed expects "
      "four parts parameters, separated by semicolons";
  string typeErr = "ERROR! Operator ffeed expects "
      "fileName: string, filePath: text, "
      "[rowNum: int] [fileSuffix: int]; "
      "[typeNodeIndex: int]; "
      "[producerIndex: int, targetIndex: int, attemptTimes: int]";
  string err1 = "ERROR! File name should NOT be empty!";
  string err2 = "ERROR! Type file is NOT exist!\n";
  string err3 = "ERROR! A tuple relation type list is "
      "NOT contained in file: ";
  string err4 = "ERROR! Infeasible evaluation in TM for attribute ";
  string err5 = "ERROR! Prefix parameter expects fileName: string";
  string err6 = "ERROR! Basic parameters expect "
      "filePath: text, [rowNum: int] [fileSuffix: int] ";
  string err7 = "ERROR! Type remote parameter expects "
      "[typeNodeIndex: int]; ";
  string err8 = "ERROR! Remote node for type file is out of range.";
  string err9 = "ERROR! Data remote parameters expect "
      "[producerIndex: int, targetIndex: int, attemptTimes: int]";
  string err10 = "ERROR! The slave list file does not exist."
      "Is $PARALLEL_SECONDO_SLAVES correctly set up ?";
  string err11 = "ERROR! Copy remote type file fail.";


  if (l.length() != 4)
    return l.typeError(lenErr);

  NList fn = l.first();
  pType = fn.first();
  pValue = fn.second();
  if (!pType.isSymbol(CcString::BasicType()))
    return l.typeError(err5);
  NList fnList;
  if (!QueryProcessor::GetNLArgValueInTM(pValue, fnList))
    return l.typeError(err4 + "fileName");
  string fileName = fnList.str();
  if (0 == fileName.length())
    return l.typeError(err1);

  NList bp = l.second();  //basic parameters
  pType = bp.first();
  pValue = bp.second();
  int bpLen = pType.length();
  if (bpLen < 1 || bpLen > 3)
    return l.typeError(err6);
  if (!pType.first().isSymbol(FText::BasicType()))
    return l.typeError(err6);
  if (bpLen > 1)
  {
    if (!pType.second().isSymbol(CcInt::BasicType()))
      return l.typeError(err6);
    if (bpLen == 3 &&
        !pType.third().isSymbol(CcInt::BasicType()))
      return l.typeError(err6);
  }

  NList fpList;
  if (!QueryProcessor::GetNLArgValueInTM(pValue.first(), fpList))
    return l.typeError(err4 + "filePath");
  string filePath = fpList.str();
  filePath = getLocalFilePath(filePath, fileName, "_type");

  NList tr = l.third();
  pType = tr.first();
  int tnIndex = -1;
  if (!pType.isEmpty())
  {
    if (pType.length() > 1 ||
        !pType.first().isSymbol(CcInt::BasicType()))
      return l.typeError(err7);
    trMode = true;
    pValue = tr.second();
    tnIndex = pValue.first().intval();
  }

  NList dr = l.fourth();
  pType = dr.first();
  if (!pType.isEmpty())
  {
    if (pType.length() != 3)
      return l.typeError(err9);
    if (!pType.first().isSymbol(CcInt::BasicType()) ||
        !pType.second().isSymbol(CcInt::BasicType()) ||
        !pType.third().isSymbol(CcInt::BasicType()))
      return l.typeError(err9);
    drMode = true;
  }

  if (tnIndex > 0)
  {
    //copy the type file from remote to here
    clusterInfo *ci = new clusterInfo();
    if (!ci->isOK())
      return l.typeError(err10);

    int sLen = ci->getLines();
    if (tnIndex > sLen)
    {
      ci->print();
      return l.typeError(err8);
    }

    string rPath = ci->getRemotePath(tnIndex, fileName, false, false);
    FileSystem::AppendItem(rPath, fileName + "_type");
    cerr << "Copy the type file from <-" << "\t" << rPath << endl;
    if (0 != system((scpCommand + rPath + " " + filePath).c_str()))
      return l.typeError(err11);
  }

  ListExpr relType;
  if (!nl->ReadFromFile(filePath, relType))
    return l.typeError(err2 + filePath);
  if (!listutils::isRelDescription(relType))
    return l.typeError(err3 + filePath);
  NList streamType =
      NList(NList(Symbol::STREAM()),
      NList(NList(relType).second()));

  return streamType.listExpr();
}

/*
6.4 Value mapping

*/
int FFeedValueMap(Word* args, Word& result,
    int message, Word& local, Supplier s)
{
  string relName, path, fileSuffix = "";
  FFeedLocalInfo* ffli = 0;
  int prdIndex = -1, tgtIndex = -1;
  int attTimes = 0;

  switch(message)
  {
    case OPEN: {
      if (!((CcString*)args[0].addr)->IsDefined()){
        cerr << "File Name string is undefined." << endl;
        return 0;
      }
      else{
        relName = ((CcString*)args[0].addr)->GetValue();
      }

      Supplier bspNode = args[1].addr,
               drpNode = args[3].addr;

      path =  ((FText*)qp->Request(
          qp->GetSupplierSon(bspNode, 0)).addr)->GetValue();
      int bspLen = qp->GetNoSons(bspNode);
      int idx = 1;
      while (idx < bspLen )
      {
        int index = ((CcInt*)qp->Request(
            qp->GetSupplierSon(bspNode, idx)).addr)->GetValue();
        if (index >= 0)
          fileSuffix += ("_" + int2string(index));
        idx++;
      }

      if (qp->GetNoSons(drpNode) == 3)
      {
        prdIndex = ((CcInt*)qp->Request(
            qp->GetSupplierSon(drpNode, 0)).addr)->GetValue();
        tgtIndex = ((CcInt*)qp->Request(
            qp->GetSupplierSon(drpNode, 1)).addr)->GetValue();
        attTimes = ((CcInt*)qp->Request(
            qp->GetSupplierSon(drpNode, 2)).addr)->GetValue();
      }

      string filePath = path;
      filePath =
          getLocalFilePath(filePath, relName, fileSuffix, false);

      ffli = (FFeedLocalInfo*) local.addr;
      if (ffli) delete ffli;
      ffli = new FFeedLocalInfo(qp->GetType(s));

      if (ffli->fetchBlockFile(
          relName , fileSuffix, filePath,
          prdIndex, tgtIndex, attTimes))
      {
        ffli->returned = 0;
        local.setAddr(ffli);
      }

      return 0;
    }
    case REQUEST: {
      ffli = (FFeedLocalInfo*)local.addr;

      if (!ffli)
        return CANCEL;

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
      if (!ffli)
        return CANCEL;
      else
      {
        if (ffli->tupleBlockFile){
          ffli->tupleBlockFile->close();
          delete ffli->tupleBlockFile;
          ffli->tupleBlockFile = 0;
        }
      }

      return 0;  //must return
    }

    case CLOSEPROGRESS: {
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


Operator ffeedOp(FFeedInfo(), FFeedValueMap, FFeedTypeMap);

/*
6.5 Implementation of FFeedLocalInfo methods

*/

bool FFeedLocalInfo::isLocalFileExist(string fp)
{
  if (fp.length() != 0)
  {
    if (FileSystem::IsDirectory(fp))
    {
      cerr << "ERROR!The denoted file is a directory." << endl;
      return false;
    }
    return FileSystem::FileOrFolderExists(fp);
  }
  return false;
}

/*
  * pdi: producer node index
  * tgi: target node index


*/
bool FFeedLocalInfo::fetchBlockFile(
    string fileName, string fileSuffix, string filePath,
    int pdi, int tgi, int att)
{
  //Fetch the binary file from remote machine.
  bool fileFound = false;
  string pdrIP = "", tgtIP = "";
  clusterInfo *ci = 0;

  string localFilePath = filePath;
  FileSystem::AppendItem(localFilePath, fileName + fileSuffix);

/*
Detect whether the file is exist or not.
If the file exists, the fileFound is set as true,
and the filePath contains the complete local path of the file.
Or else, the fileFound is false.

*/
  if (pdi < 0)
  {
    //Fetch the file in the local machine
    fileFound = isLocalFileExist(localFilePath);
  }
  else
  {
    //Fetch the file in a remote machine
    ci = new clusterInfo();
    if(!ci->isOK())
    {
      cerr << "ERROR!The PARALLEL_SECONDO_SLAVES list is not "
          "correctly set up." << endl;
      return false;
    }
    if (tgi > ci->getLines())
    {
      cerr << "ERROR!The first target's index is out of "
          "the range of the slave list." << endl;
      ci->print();
      return false;
    }

    pdrIP = ci->getIP(pdi);
    string subFolder = "";

    while (!fileFound && (att-- > 0))
    {
/*
In case the duplicated file's name conflicts with files produced
locally by the remote machine itself,
duplicated files' names are ended with its host's IP address.
When feeding the file from a non-producer node,
the file's name has to be extended with the producer's IP address.

*/
      string rFilePath;
      tgtIP = ci->getIP(tgi, true);
      bool attachProducerIP;
      if (tgi == pdi)
        attachProducerIP = false;
      else
        attachProducerIP = true;
      rFilePath = ci->getRemotePath(tgi, fileName, true, false,
          false, attachProducerIP, pdrIP);
      FileSystem::AppendItem(rFilePath, fileName + fileSuffix);
      if (ci->getLocalIP().compare(tgtIP) == 0)
      {
        // Fetch the file in local, but is produced by another node
        localFilePath = rFilePath;
        fileFound = isLocalFileExist(localFilePath);
      }
      else
      {
        // Fetch the file in remote machine
        int detectTimes = MAX_COPYTIMES;
        string qStr = "ssh " + tgtIP +
            " ls " + rFilePath + " 2>/dev/null";
        while (!fileFound && (detectTimes-- > 0))
        {
/*
We detect the remote file through a shell ssh command,
and the command may fail if the network is too busy.
Therefore we have to try several times.

*/
          FILE *fs;
          char qBuf[1024];
          memset(qBuf, '\0', sizeof(qBuf));
          fs = popen(qStr.c_str(), "r");
/*
popen function is not a standard C function, and it may return
a NULL value because of the failure of allocating memory.
Some systems have a limitation of opened files simultaneously.

*/
          if (fs == NULL)
            perror(("popen fail! Detecting the remote file: "
                + tgtIP + ":" + rFilePath ).c_str());
          else if (fgets(qBuf, sizeof(qBuf), fs) != NULL)
          {
            fileFound = true;
            pclose(fs);
          }
        }
      }

      if (!fileFound)
      {
        cerr << "Warning! Cannot detect the file "
            << tgtIP << ":" << rFilePath << endl;
        tgi++;
        continue;
      }
      else
      {
        if (ci->getLocalIP().compare(tgtIP) != 0)
        {
/*
Copy a file only when it's stored in a remote node.
If there is a file exist with a same file name kept in local node,
then this file will be viewed as an unavailable file, and be deleted.

In some cases, when the the file is stored in the local node,
then nothing to be done, since the file is already found.

*/
          if (isLocalFileExist(localFilePath))
          {
            cerr << "Delete the local file with the same name\n";
            FileSystem::DeleteFileOrFolder(localFilePath);
          }
          int copyTimes = MAX_COPYTIMES;
          do{
            if (0 != system((scpCommand + tgtIP + ":" + rFilePath +
                " " + localFilePath).c_str()))
              cerr << "Warning! Copy remote file fail." << endl;
          }while ((--copyTimes > 0) &&
              !FileSystem::FileOrFolderExists(localFilePath));
          if (copyTimes < 0)
          {
            cerr << "Warning! Cannot copy the remote file: "
                << rFilePath << endl;
            fileFound = false;
          }
        }
      }
    }
  }

  if (!fileFound)
  {
    cerr << "\nERROR! File " << localFilePath
         << " is not exist and cannot be remotely fetched.\n\n\n";
    return false;
  }

  tupleBlockFile = new ifstream(localFilePath.c_str(), ios::binary);
  if (!tupleBlockFile->good())
  {
    cerr << "ERROR! Read file " << localFilePath << " fail.\n\n\n";
    tupleBlockFile = 0;
    return false;
  }

  //Catch the file, and read the description list
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

  return true;
}

/*
7 Operator ~hadoopjoin~

This operator carries out a Hadoop join operation in Secondo.

The operator maps:

----
   ( (stream(tuple(T1))) x (stream(tuple(T2)))
    x array(string) x int x int x string
    x (map stream(tuple(T1)) stream(tuple(T2))
           stream(tuple(T1 T2))))
    -> stream(tuple((mIndex int)(pIndex int)))
----

This operator evaluates the parallel join operation
in Secondo by calling a generic Hadoop join program.
The operator only works in the Secondo system
which is deployed in a cluster which has a Hadoop system,
and the Secondo Monitors on all nodes that belong to the cluster
have been started already.
The results of the operation are distributed in nodes as files,
with argument ~resultName~ as file name.
And the operator outputs a tuple stream to indicate these files' places.
The tuple stream contains two fields: mIndex and pIndex.
The mIndex denotes which node have the result file,
and the pIndex denotes which part of the complete result is inside
the file.

The operator contains 7 parameters in total:
mq1Stream, mq2Stream, machineArr, masterIndex, rtNum,
resultName and rqMap.

The mq1Stream, mq2Stream and rqMap are Secondo queries.
By using the feature of ~SetUsesArgsInTypeMapping~,
we can get the nested list of these queries, and send them to
Hadoop program as arguments.
Then these queries are merged with some fixed nested list type
queries written in the Hadoop program already, and are sent to multiple
remote Secondo monitors to run.

The machineArr is an array object that is kept
in all Secondo databases of the cluster's nodes.
This array contains the complete list of the nodes' names in the
cluster, and the parameter masterIndex which is also kept in
all Secondo databases, indicates which node in the array is the
master node.

The parameter rtNum is used to define how many reduce tasks we want
to use in the Hadoop job. The number of the map tasks are defined
by the amount of slave nodes in the machineArr.


Update in 10/06/2010
Replace the requirement for Partition attribute
by denoting a partition basis attribute,
if this attribute's type provides the HashValue function
required by ~fdistribute~ operator.

At the same time,
the machineArray and masterIndex are not required any more,
since we use the PARALLEL\_SECONDO\_SLAVES list.

Now the operator maps

----
stream(tuple(T1) x stream(tuple(T2))
x partAttr1 x partAttr2 x partitionNum x resultName
x (map stream(tuple(T1)) stream(tuple(T2) stream(tuple(T1 T2)))))
-> stream(tuple((MIndex int)(PIndex int)))
----

*/

struct HdpJoinInfo : OperatorInfo {

  HdpJoinInfo() : OperatorInfo(){
    name = "hadoopjoin";
    signature =
        "(stream(tuple(T1)) x stream(tuple(T2)) x "
        " partAttr1 x partAttr2 x int x string x "
        " (map stream(tuple(T1)) "
        "  stream(tuple(T2) stream(tuple(T1 T2)))))"
        "-> stream(tuple(int int))";
    syntax = "stream(tuple(T1) stream(tuple(T2)) "
        "hadoopjoin[partAttr1, partAttr2, partitionNum, resultName; "
        "joinQuery]";
    meaning =
        "Evaluating a join operation on parallel Secondo "
        "by invoking a generic Hadoop join job. "
        "The join procedure is processed by several computers "
        "within a cluster simultaneously. "
        "The result tuples are encapsulated into several files, "
        "stored on different nodes. "
        "The output stream of this operator denotes the locations "
        "of these result data files.";
  }
};

/*
7.1 Type mapping

*/

ListExpr hdpJoinTypeMap(ListExpr args)
{
  string lengErr = "Operator hadoopjoin expects a list "
      "of seven arguments. ";
  string typeErr = "operator hadoopjoin expects "
      "(stream(tuple(T1)), stream(tuple(T2)), "
      "partAttr1, partAttr2, int, string,"
      "(map (stream(tuple(T1))) stream(tuple(T2))"
      "  stream(tuple(T1 T2))) )";
  string err1 =
      "ERROR! Infeasible evaluation in TM for attribute ";

  NList l(args);

  if (l.length() != 7)
    return l.typeError(lengErr);

  string ss[2] = {"", ""};  // nested list of input streams
  string an[2] = {"", ""};  // attribute name
  //Both input are tuple streams,
  //and the partition attribute is included in respective stream
  for (int argIndex = 1; argIndex <= 2; argIndex++)
  {
    NList attrList;
    NList streamList = l.elem(argIndex).first();
    if (!streamList.checkStreamTuple(attrList))
      return l.typeError(typeErr);

    NList partAttr = l.elem(argIndex + 2).first();
    if (!partAttr.isAtom())
      return l.typeError(typeErr);

    ListExpr attrType;
    string attrName = partAttr.str();
    int attrIndex = listutils::findAttribute(
        attrList.listExpr(), attrName, attrType);
    if (attrIndex <= 0)
      return l.typeError(typeErr);

    ss[argIndex - 1] = l.elem(argIndex).second().convertToString();
    an[argIndex - 1] = attrName;
  }

  // Partition scale number
  if (!l.fifth().first().isSymbol(CcInt::BasicType()))
    return l.typeError(typeErr);

  // Result file name
  if (!l.sixth().first().isSymbol(CcString::BasicType()))
    return l.typeError(typeErr);

  NList rnList;
  if (!QueryProcessor::GetNLArgValueInTM(
        l.sixth().second(), rnList))
    return l.typeError(err1 + " resultName");
  string resultName = rnList.str();

  string mapStr = l.elem(7).second().fourth().convertToString();
  NList mapList = l.elem(7).first();

  NList attrAB;
  if (! (mapList.first().isSymbol(Symbol::MAP())
      && mapList.fourth().checkStreamTuple(attrAB)))
    return l.typeError(typeErr);

    // Write the join result type into local default path,
    // in case the following operators need.
    NList joinResult =
        NList(NList(Relation::BasicType()),
              NList(NList(Tuple::BasicType()), NList(attrAB)));
    string typeFileName =
        getLocalFilePath("", resultName, "_type", true);
    ofstream typeFile(typeFileName.c_str());
    if (!typeFile.good())
      cerr << "Create typeInfo file Result_type "
          "in default parallel path error!" << endl;
    else
    {
      //The accepted input is a stream tuple
      typeFile << joinResult.convertToString() << endl;
      typeFile.close();
    }
    cerr << "\nSuccess created type file: "
        << typeFileName << endl;

    // result type
    NList a1(NList("MIndex"), NList(CcInt::BasicType()));
    NList a2(NList("PIndex"), NList(CcInt::BasicType()));

    NList result(
        NList(Symbols::STREAM()),
          NList(NList(Tuple::BasicType()),
            NList(
              NList(NList("MIndex"), NList(CcInt::BasicType())),
              NList(NList("PIndex"), NList(CcInt::BasicType())))));

    NList appList;
    appList.append(NList(ss[0], true, true));
    appList.append(NList(ss[1], true, true));
    appList.append(NList(mapStr, true, true));
    appList.append(NList(an[0], true, false));
    appList.append(NList(an[1], true, false));

    return NList(NList(Symbol::APPEND()), appList, result).
        listExpr();
}

/*
7.2 Value mapping

*/

int hdpJoinValueMap(Word* args, Word& result,
    int message, Word& local, Supplier s)
{
  hdpJoinLocalInfo* hjli = 0;

  switch(message)
  {
    case OPEN:{
      //0 Set the parameters
      //0.1 assume the operation happens on
      //all nodes' Secondo databases with a same name
      string dbName =
          SecondoSystem::GetInstance()->GetDatabaseName();

      //0.2 set other arguments
      int rtNum = ((CcInt*)args[4].addr)->GetIntval();
      string rName = ((CcString*)args[5].addr)->GetValue();
      string mrQuery[3] = {
          ((FText*)args[7].addr)->GetValue(),
          ((FText*)args[8].addr)->GetValue(),
          ((FText*)args[9].addr)->GetValue()
      };
      string attrName[2] = {
          ((CcString*)args[10].addr)->GetValue(),
          ((CcString*)args[11].addr)->GetValue()
      };

      //1 evaluate the hadoop program
      stringstream queryStr;
      queryStr << "hadoop jar HdpSec.jar dna.HSJoin \\\n"
        << dbName << " \\\n"
        << "\"" << tranStr(mrQuery[0], "\"", "\\\"") << "\" \\\n"
        << "\"" << attrName[0] << "\" \\\n"
        << "\"" << tranStr(mrQuery[1], "\"", "\\\"") << "\" \\\n"
        << "\"" << attrName[1] << "\" \\\n"
        << "\"" << tranStr(mrQuery[2], "\"", "\\\"") << "\" \\\n"
        << rtNum << " " << rName << endl;
      int rtn;
//      cout << queryStr.str() << endl;   //Used for debug only
      rtn = system("hadoop dfs -rmr OUTPUT");
      rtn = system(queryStr.str().c_str());

      //2 get the result file list
      if (hjli)
        delete hjli;
      hjli = new hdpJoinLocalInfo(s);

      FILE *fs;
      char buf[MAX_STRINGSIZE];
//      fs = popen("cat pjResult", "r");  //Used for debug only
      fs = popen("hadoop dfs -cat OUTPUT/part*", "r");
      if (NULL != fs)
      {
        while(fgets(buf, sizeof(buf), fs))
        {
          stringstream ss;
          ss << buf;
          istringstream iss(ss.str());
          int mIndex, pIndex;
          iss >> pIndex >> mIndex;
          hjli->insertPair(make_pair(mIndex, pIndex));
        }
        pclose(fs);
        hjli->setIterator();
        local.setAddr(hjli);
      }
      return 0;
    }
    case REQUEST:{
      if (0 == local.addr)
        return CANCEL;
      hjli = (hdpJoinLocalInfo*)local.addr;
      result.setAddr(hjli->getTuple());

      if (result.addr)
        return YIELD;
      else
        return CANCEL;
    }
    case CLOSE:{
      if (0 == local.addr);
        return CANCEL;
      hjli = (hdpJoinLocalInfo*)local.addr;
      delete hjli;

      return 0;
    }
  }

  //should never be here
  return 0;
}

Operator hadoopjoinOp(HdpJoinInfo(),
                      hdpJoinValueMap,
                      hdpJoinTypeMap);

/*
8 Operator ~fdistribute~

The operator maps

----
stream(tuple(...))
x fileName x path x attrName
x [nBuckets] x [KPA]
-> stream(tuple(fileSufix, value))
----

~fdistribute~ partitions a tuple stream into several binary files
based on a specific attribute value, along with a linear scan.
These files could be read by ~ffeed~ operator.
This operator is used to replace the expensive ~groupby~ + ~fconsume~ operations,
which need sort the tuple stream first.

The operator accepts at least 4 parameters:
a tuple stream, files' base name, files' path and keyAttributeName.
The first three are same as ~fconsume~ operator,
the fourth parameter defines the key attribute
by whose hash value tuples are partitioned.

If the fifth parameter nBuckets is given, then tuples are
evenly partitioned to buckets based on modulo function,
or else these tuples are partitioned based on
keyAttribute values' hash numbers directly,
which may partitions these tuples NOT evenly.

It's also possible to accept the sixth parameter,
KPA (Keep Partition Attribute),
which indicates whether the key attribute is removed.
By default it's false, i.e. remove that key attribute,
just like what the ~distribute~ operator does.
But if it's set to be true, then the key attribute will
stay in the result files.

In 13/5/2011, enable ~fdistribute~ operation with duplication function.
As we need use ~fdistribute~ in the generic hadoop operation's map step,
it's necessary to use ~fdistribute~ operator to duplicate its result
files into candidate nodes, to meet the requirement of fault-tolerance
feature.

Same as ~fconsume~ and ~ffeed~ operators, the duplicate parameters
are optional, and are separated from the basic parameters by semicolons.
Now the operator maps

----
stream(tuple(...))
x fileName x path x attrName
x [nBuckets] x [KPA]
x [typeNodeIndex1] x [typeNodeIndex2]
x [targetIndex x dupTimes ]
-> stream(tuple(fileSufix, value))
----

In 8/6/2011, extend the fdistribute with another new parameter, rowNum.
Since this should also be an optional parameter,
and it may be confused with the nBuckets,
I decided to further divide the parameter list to 5 parts,
set the rowNum after the attrName, as an optional parameter,
and group nBuckeets and KPA as another group of parameters.
Now the operator maps

----
stream(tuple(...))
x fileName x path x attrName x [rowNum] ;
x [nBuckets] x [KPA] ;
x [typeNodeIndex1] x [typeNodeIndex2] ;
x [targetIndex x dupTimes ]
-> stream(tuple(fileSufix, value))
----


8.0 Specification

*/

struct FDistributeInfo : OperatorInfo {

  FDistributeInfo() : OperatorInfo()
  {
    name = "fdistribute";
    signature = "stream(tuple(a1 ... ai ... aj)) "
        "x string x text x symbol x [int] x [int] x [bool] "
        "x [int] x [int] x [ int x int ]"
        "-> stream(tuple( ... )) ";
    syntax =
        "stream(tuple(a1 ... ai ... aj)) "
        " fdistribute[ fileName, path, partitionAttr, [rowNum];"
        " [bucketNum], [KPA]; "
        " [typeNode1], [typeNode2]; "
        " [targetIndex,  dupTimes] ]";
    meaning =
        "Export a stream of tuples into binary data files "
        "that can be read by ffeed operator, and write the schema "
        "of the stream into a text type file. "
        "Tuples are distributed into different data files "
        "based on the hash value of the given partition attribute, "
        "if the attribute's type provides the HashValue function. "
        "Data files are distinguished from each other "
        "by using these hash values as their name's suffices. "
        "If the bucketNum is given, then the tuples are re-hashed "
        "by the bucketNum again to achieve an even partition. "
        "Users can optionally keeping the partition attribute value "
        "by setting the value of KPA(Keep Partition Attribute) "
        "as true,  which is set as false by default. "
        "Both type file and data file can be duplicated "
        "to some remote machines, which are listed in a list file "
        "indicated by PARALLEL_SECONDO_SLAVES environment variable. "
        "Detail explanations are described in the attached "
        "README.pdf along with the HadoopParallel algebra.";
  }

};

/*
8.1 Type mapping

*/
ListExpr FDistributeTypeMap(ListExpr args)
{
  NList l(args);
  string lenErr = "ERROR!Operator expects 5 parts arguments.";
  string typeErr = "ERROR!Operator expects "
      "(stream(tuple(a1, a2, ..., an))) "
      "x string x text x ai x [int] x [int] x [bool] "
      "x [int] x [int] x [ int x int ] ";
  string attErr = "ERROR!Operator cannot find the "
      "partition attribute: ";
  string err4 = "ERROR!Basic arguments expect "
      "fileName: string, filePath: text, attrName: ai"
      "[rowNum: int]";
  string err11 = "ERROR!Parition mode expects "
      "{nBuckets: int}, {keepPartAttr: bool}";
  string err5 = "ERROR!Type remote nodes expects "
      "[[typeNodeIndex: int], [typeNodeIndex2: int]]";
  string err6 = "ERROR!Data remote nodes expects "
        "[targetNode:int, duplicateTimes: int]";

  string err1 = "ERROR!Infeasible evaluation in TM for attribute ";
  string err2 = "ERROR!The file name should NOT be empty!";
  string err3 = "ERROR!Fail by openning file: ";
  string err7 = "ERROR!Infeasible evaluation in TM for attribute: ";
  string err8 = "ERROR!The slave list file does not exist."
        "Is $PARALLEL_SECONDO_SLAVES correctly set up ?";
  string err9 = "ERROR!Remote node for type file is out of range";
  string err10 = "ERROR!Remote duplicate type file fail.";

  if (l.length() != 5)
    return l.typeError(lenErr);

  NList pType, pValue;

  //First part argument (including stream(tuple(...)) )
  NList attrsList;
  if (!l.first().first().checkStreamTuple(attrsList))
    return l.typeError(typeErr);

  NList bpList = l.second();
  //Basic parameters (including string, text, symbol, [int])
  pType = bpList.first();
  pValue = bpList.second();
  int bpLen = pType.length();

  if (bpLen < 3 || bpLen > 4)
    return l.typeError(err4);

  // File name
  if (!pType.first().isSymbol(CcString::BasicType()))
    return l.typeError(err4);
  NList fnList;
  if (!QueryProcessor::GetNLArgValueInTM(pValue.first(), fnList))
    return l.typeError(err1 + "fileName");
  string filePrefix = fnList.str();
  if (0 == filePrefix.length())
    return l.typeError(err2);

  // File path
  if (!pType.second().isSymbol(FText::BasicType()))
    return l.typeError(err4);
  NList fpList;
  if (!QueryProcessor::GetNLArgValueInTM(pValue.second(), fpList))
    return l.typeError(err1 + "filePath");
  string filePath = fpList.str();

  // Partition attribute
  if (!pType.third().isAtom())
    return l.typeError(typeErr + "\n" + err4);
  string attrName = pValue.third().str();
  ListExpr attrType;
  int attrIndex = listutils::findAttribute(
      attrsList.listExpr(), attrName, attrType);
  if (attrIndex < 1)
    return l.typeError(attErr + attrName);

  //Optional row number
  if ( bpLen == 4 )
    if (!pType.fourth().isSymbol(CcInt::BasicType()))
      return l.typeError(err4);

  bool evenMode = false;
  bool setKPA = false, KPA = false;
  NList pmList = l.third();
  //Partition mode (including [nBuckets], [KPA])
  pType = pmList.first();
  pValue = pmList.second();
  int pmLen = pType.length();
  if (pmLen < 0 || pmLen > 2)
    return l.typeError(err11);
  if (1 == pmLen)
  {
    if (pType.first().isSymbol(CcInt::BasicType()))
      evenMode = true;
    else if (pType.first().isSymbol(CcBool::BasicType()))
    {
      setKPA = true;
      KPA = pValue.first().boolval();
    }
    else
      return l.typeError(err11);
  }
  else if (2 == pmLen)
  {
    if (!pType.first().isSymbol(CcInt::BasicType()) ||
        !pType.second().isSymbol( CcBool::BasicType()))
      return l.typeError(err11);
    else
    {
      evenMode = true;
      setKPA = true;
      KPA = pValue.second().boolval();
    }
  }

  //Remove the attribute used for partition the relation
  NList newAL; //new attribute list
  if (KPA)
    newAL = attrsList;
  else
  {
    NList rest = attrsList;
    while (!rest.isEmpty())
    {
      NList elem = rest.first();
      rest.rest();
      if (elem.first().str() != attrName)
        newAL.append(elem);
    }
  }

  //Create the type file in local disk
  filePath = getLocalFilePath(filePath, filePrefix, "_type");
  ofstream typeFile(filePath.c_str());
  NList resultList =
          NList(NList(Relation::BasicType()),
                NList(NList(Tuple::BasicType()), newAL));
  if (!typeFile.good())
    return l.typeError(err3 + filePath);
  else
  {
    typeFile << resultList.convertToString() << endl;
    cerr << "Created type file " << filePath << endl;
  }
  typeFile.close();

  clusterInfo* ci = 0;
  NList trList = l.fourth();
  pType = trList.first();
  int tNode[2] = {-1, -1};
  if (!pType.isEmpty())
  {
    //Get the type index and duplicate the type file.
    if (pType.length() > 2)
      return l.typeError(err5);
    while (!pType.isEmpty())
    {
      if (!pType.first().isSymbol(CcInt::BasicType()))
        return l.typeError(err5);
      pType.rest();
    }

    pValue = trList.second();
    int cnt = 0;
    while(!pValue.isEmpty())
    {
      NList nList;
      if (!QueryProcessor::GetNLArgValueInTM(pValue.first(), nList))
        return l.typeError( err7 + " type node index");
      tNode[cnt++] = nList.intval();
      pValue.rest();
    }

    //scp filePath .. IP:loc/typeFileName
    ci = new clusterInfo();
    if (!ci->isOK())
      return l.typeError(err8);
    int sLen = ci->getLines();
    for (int i = 0; i < 2; i++)
    {
      if (tNode[i] > 0)
      {
        if (tNode[i] > sLen)
        {
          ci->print();
          return l.typeError(err9);
        }
        string rPath = ci->getRemotePath(tNode[i], filePrefix);
        cerr << "Copy the type file to -> \t" << rPath << endl;
        if (0 != system(
             (scpCommand + filePath + " " + rPath).c_str()))
          return l.typeError(err10);
      }
    }
  }

  NList drList = l.fifth();
  pType = drList.first();
  if (!pType.isEmpty())
  {
    if(pType.length() != 2)
      return l.typeError(err6);
    if (!pType.first().isSymbol(CcInt::BasicType()) ||
        !pType.second().isSymbol(CcInt::BasicType()))
      return l.typeError(err6);
  }

  NList outAttrList =
           NList(NList(NList("Suffix"), NList(CcInt::BasicType())),
                 NList(NList("TupNum"), NList(CcInt::BasicType())));
  NList outList = NList().tupleStreamOf(outAttrList);

  return NList(NList(Symbol::APPEND()),
               NList(
                 NList(attrIndex),
                 NList(
                   NList(NList(
                     Tuple::BasicType()), newAL).convertToString(),
                     true, true)),
               outList).listExpr();
}

/*
8.2 Value mapping

*/
int FDistributeValueMap(Word* args, Word& result,
    int message, Word& local, Supplier s)
{
  string relName, path;
  FDistributeLocalInfo* fdli = 0;
  Word elem;

  switch(message)
  {
    case OPEN: {
      SecondoCatalog* sc = SecondoSystem::GetCatalog();
      qp->Open(args[0].addr);

      Supplier bspList = args[1].addr,
               ptmList = args[2].addr,
               drpList = args[4].addr;

      relName = ((CcString*)qp->Request(
          qp->GetSupplierSon(bspList,0)).addr)->GetValue();
      path = ((FText*)qp->Request(
          qp->GetSupplierSon(bspList,1)).addr)->GetValue();

      int rowNum = -1;
      int bspLen = qp->GetNoSons(bspList);
      if (4 == bspLen)
        rowNum = ((CcInt*)qp->Request(
            qp->GetSupplier(bspList,3)).addr)->GetValue();

      bool evenMode = false, kpa = false;
      int nBucket = 0;
      int ptmLen = qp->GetNoSons(ptmList);
      if (1 == ptmLen)
      {
        ListExpr ptList = qp->GetType(qp->GetSupplierSon(ptmList,0));
        if (nl->IsEqual(ptList, CcBool::BasicType()))
          kpa = ((CcBool*)qp->Request(
              qp->GetSupplierSon(ptmList,0)).addr)->GetValue();
        else
        {
          evenMode = true;
          nBucket = ((CcInt*)qp->Request(
              qp->GetSupplierSon(ptmList,0)).addr)->GetValue();
        }
      }
      else if (2 == ptmLen)
      {
        evenMode = true;
        nBucket = ((CcInt*)qp->Request(
            qp->GetSupplierSon(ptmList,0)).addr)->GetValue();
        kpa = ((CcBool*)qp->Request(
            qp->GetSupplierSon(ptmList,1)).addr)->GetValue();
      }

      int attrIndex =
            ((CcInt*)args[5].addr)->GetValue() - 1;

      string inTupleTypeStr =
               ((FText*)args[6].addr)->GetValue();
      ListExpr inTupleTypeList;
      nl->ReadFromString(inTupleTypeStr, inTupleTypeList);
      inTupleTypeList = sc->NumericType(inTupleTypeList);

      int drpLen = qp->GetNoSons(drpList);
      int dupTgtIndex = -1, dupTimes = -1;
      if (2 == drpLen)
      {
        dupTgtIndex = ((CcInt*)qp->Request(
            qp->GetSupplierSon(drpList, 0)).addr)->GetValue();
        dupTimes    = ((CcInt*)qp->Request(
            qp->GetSupplierSon(drpList, 1)).addr)->GetValue();
      }

      fdli = (FDistributeLocalInfo*) local.addr;
      if (fdli) delete fdli;
      ListExpr resultTupleList = GetTupleResultType(s);
      fdli = new FDistributeLocalInfo(
               relName, rowNum, path, nBucket, attrIndex, kpa,
               resultTupleList, inTupleTypeList,
               dupTgtIndex, dupTimes);
      local.setAddr(fdli);

      //Write tuples to files completely
      qp->Open(args[0].addr);
      qp->Request(args[0].addr, elem);
      while(qp->Received(args[0].addr))
      {
        if (!fdli->insertTuple(elem))
          break;

        qp->Request(args[0].addr, elem);
      }
      qp->Close(args[0].addr);
      if (!fdli->startCloseFiles())
        return CANCEL;
      return 0;
    }
    case REQUEST: {
      fdli = static_cast<FDistributeLocalInfo*>(local.addr);
      if (!fdli)
        return CANCEL;

      //Return the counters of each file
      Tuple* tuple = fdli->closeOneFile();
      if (tuple)
      {
        result.setAddr(tuple);
        return YIELD;
      }
      return CANCEL;
    }
    case CLOSE: {
      fdli = static_cast<FDistributeLocalInfo*>(local.addr);
      if (fdli)
        delete fdli;
      local.addr = 0;
      return 0;
    }
  }
  return 0;
}

/*
8.3 Implementation of FDistributeLocalInfo methods

*/

FDistributeLocalInfo::FDistributeLocalInfo(
    string _bn, int _rn, string _pt, int _nb, int _ai, bool _kpa,
    ListExpr _rtl, ListExpr _itl,
    int _di, int _dt)
: nBuckets(_nb), attrIndex(_ai), kpa(_kpa), tupleCounter(0),
  rowNumSuffix(""), firstDupTarget(_di), dupTimes(_dt),
  localIndex(0), cnIP(""),
  ci(0), copyList(0)
{
  string fnSfx = "";
  if ( _rn >= 0 )
    rowNumSuffix = "_" + int2string(_rn);
  fileBaseName = _bn;
  filePath = getLocalFilePath(_pt, _bn, rowNumSuffix, false);
  resultTupleType = new TupleType(nl->Second(_rtl));
  exportTupleType = new TupleType(_itl);
}

bool FDistributeLocalInfo::insertTuple(Word tupleWord)
{
  Tuple *tuple = static_cast<Tuple*>(tupleWord.addr);
  size_t fileSfx = HashTuple(tuple);
  bool ok = true;

  map<size_t, fileInfo*>::iterator mit;
  mit = fileList.find(fileSfx);
  fileInfo* fp;
  if (mit != fileList.end())
    fp = (*mit).second;
  else
  {
    fp = new fileInfo(fileSfx, filePath, fileBaseName,
        exportTupleType->GetNoAttributes(), rowNumSuffix);
    fileList.insert(pair<size_t, fileInfo*>(fileSfx, fp));
  }
  ok = openFile(fp);

  if (!(ok &&
        fp->writeTuple(tuple, tupleCounter,
                       attrIndex, exportTupleType, kpa)))
    cerr << "Block File " << fp->getFilePath() << " Write Fail.\n";
  tupleCounter++;
  tuple->DeleteIfAllowed();

  return ok;
}

bool FDistributeLocalInfo::openFile(fileInfo* tgtFile)
{
  if (tgtFile->isFileOpen())
    return true;

  if (openFileList.size() >= MAX_FILEHANDLENUM)
  {
    //sort fileInfos according to their last tuples' indices
    sort(openFileList.begin(), openFileList.end(), compFileInfo);
    //The last one of the vector is the idler
    bool poped = false;
    while(!poped)
    {
      if (openFileList.back()->isFileOpen())
      {
        openFileList.back()->closeFile();
        poped = true;
      }
      openFileList.pop_back();
    }
  }

  bool ok = tgtFile->openFile();
  openFileList.push_back(tgtFile);
  return ok;
}

bool FDistributeLocalInfo::startCloseFiles()
{
  fit = fileList.begin();

  if (dupTimes > 0)
  {
    ci = new clusterInfo();
    if(!ci->isOK())
    {
      cerr << "ERROR!The slave list file does not exist."
      "Is $PARALLEL_SECONDO_SLAVES correctly set up ?" << endl;
      return false;
    }
    if(ci->getLines() < firstDupTarget)
    {
      cerr << "The first target node index is "
          "out of the range of slave list" << endl;
      return false;
    }

    int cLen = ci->getLines();
    copyList = new bool[cLen + 1];
    memset(copyList, false, (cLen + 1));
    int ti = firstDupTarget;
    for (int i = 0; i < dupTimes; i++, ti++)
      copyList[((ti - 1)%cLen + 1)] = true;

    localIndex = ci->getLocalNode();
    if (localIndex < 1)
    {
      cerr << "ERROR! Cannot find the local position " << endl
          << ci->getLocalIP() << ":" << ci->getLocalPath() << endl
          << "in the slave list, backup files will fail." << endl;
      ci->print();
      return false;
    }
    cnIP = ci->getIP(localIndex);
  }

  return true;
}

Tuple* FDistributeLocalInfo::closeOneFile()
{
  Tuple* tuple = 0;
  if (fit != fileList.end())
  {
    int suffix = (*fit).first;
    fileInfo* fp = (*fit).second;
    bool ok = openFile(fp);

    if ( ok )
    {
      int count = fp->writeLastDscr();
      fp->closeFile();
      tuple = new Tuple(resultTupleType);
      tuple->PutAttribute(0, new CcInt(suffix));
      tuple->PutAttribute(1, new CcInt(count));
    }

    if (!duplicateOneFile(fp))
    {
      tuple->DeleteIfAllowed();
      return 0;
    }
    fit++;
  }
  return tuple;
}

bool FDistributeLocalInfo::duplicateOneFile(fileInfo* fi)
{
  //Duplicate a file after close it.
  //if the duplicating goes wrong, it can tell the stream to stop.

  if (copyList)
  {
    if (fi->isFileOpen())
      fi->closeFile();
    string filePath = fi->getFilePath();
    int cLen = ci->getLines();
    bool keepLocal = false;
    for (int i = 1; i <= cLen; i++)
    {
      if (copyList[i])
      {
        if ((i == localIndex))
        {
          keepLocal = true;
          continue;
        }
        else
        {
          string rPath =
              ci->getRemotePath(i,fileBaseName, true, true, true, true);
          FileSystem::AppendItem(rPath,fi->getFileName());
          if (system((scpCommand + filePath + " " + rPath).c_str()))
          {
            cerr << "Copy remote file fail." << endl;
            return false;
          }
        }
      }
    }
    if (!keepLocal)
    {
      if ( 0 != (system(("rm " + filePath).c_str())))
      {
        cerr << "Delete local file " << filePath << " fail.\n";
        return false;
      }
      cerr << "Local file " << filePath << " is deleted.\n";
    }

  }
  return true;
}

Operator fdistributeOp(FDistributeInfo(),
                       FDistributeValueMap,
                       FDistributeTypeMap);

/*
9 Data Type fList

During the parallel processing, we need to indicate Secondo objects
distributed in slave nodes of the cluster. There are two situations
need to be considered:

  * Object's pieces are kept in Secondo databases, with a same name.

  * Object's pieces are kept in a series of binary files, start with a same name.

The fList is used to negotiate the second situation.

Assume a Secondo relation is divided to ~n~ * ~p~ files,
here ~n~ is the number of slave nodes of the cluster,
and ~p~ is the number of partitions that this object is divided to
on each slave node.
The complete set of these files are called as a matrix-file,
each file is called as a cell file,
and each slave node keeps a row of this matrix, i.e. ~p~ cell files.

During the MapReduce parallel processing involving fList,
each map or reduce task process one column cell files of the matrix file,
and produce one row cell files of a new matrix file to the local node.

To describe the distribution of a Secondo object, following attributes
are required in the fList:

  * objectName

  * objectType

  * nodesList

  * fileLocList

  * duplicateTimes

  * Available

The ~objectName~ is the name of the Secondo object,
also the prefix name of all cell files.
The ~objectType~ describes the schema of the Secondo object.
At present, the type must be a tuple relation.
The ~nodesList~ contains the IP addresses of all nodes in the cluster,
and the first node is viewed as the master node.
The ~fileLocList~ indicates the location of the cell files.
It uses slaves' indices in above ~nodesList~ as the indicator.
In some cases, a cell file may be duplicated into several nodes,
and the parameter ~duplicateTimes~ denotes the times of the duplication,
which must not be less than 1.
The last attribute is ~Available~, which denotes whether the last
operator which created this list is successfully performed.

*/
fList::fList(string _on, NList _tl, NList _nl,
    NList _fll, size_t _dp, bool _ia):
    objName(_on), objectType(_tl),
    nodesList(_nl), fileLocList(_fll),
    dupTimes(_dp), isAvailable(_ia),
    mrNum(0), mcNum(-1)
{
  if (isAvailable)
    verifyLocList();
}

fList::fList(fList& rhg):
    objName(rhg.getObjName()),
    objectType(rhg.getTypeList()),
    nodesList(rhg.getNodeList()),
    fileLocList(rhg.getLocList()),
    dupTimes(rhg.getDupTimes()),
    isAvailable(rhg.isAvailable),
    mrNum(rhg.getMtxRowNum()),
    mcNum(rhg.getMtxColNum())

{ }

fList::~fList()
{ }

/*
9.1 fList::In Function

The ~In~ function accepts following parameters

  * A Secondo object name.
This is a string value express the name of an exist Secondo object.
We use it to get the ~objectName~ and its type expression.
In some cases, the object may not exist, then there should be a text
file that contains the type expression exists in the local *parallel* directory.
If both the object and the type file don't exist,
then set the ~correct~ as FALSE.

  * A nodesList.
This is a list of string values, each specifies a IP address of
a node in the cluster. And the first one is viewed as the master
node by default.

  * A fileLocLMatrix
This is a nested list that composed by integer numbers,
which denotes the matrix of cell files
E.g., it may looks like this:

---- (  (1 ( 1 2 3 4 5))
        (2 ( 1 2 3 4 5))
        (3 ( 1 2 3 4 5))
        (4 ( 1 2 3 4 5)) )
----

The above example shows that a Secondo objects is divided into
a 4x5 matrix file, and is distributed to a cluster with 4 nodes.
Each node, including the master node, contains five cell files.

  * A duplicate times
This is a integer number used to tell how many duplications of a
cell file are kept inside the cluster.
At present, we adopt a simple chained declustering mechanism to
backup the duplications of the cell files.
Besides the primary node that is denoted in the fileLoc matrix,
it will be copied to (~dupTimes~ - 1) nodes that are listed after
the primary node within the nodesList.


*/
Word fList::In(const ListExpr typeInfo, const ListExpr instance,
            const int errorPos, ListExpr& errorInfo, bool& correct)
{
  Word result = SetWord(Address(0));
  string typeErr = "expect (objectName nodesList fileLocList).";
  string flocErr = "improper file location list, "
      "refer to source document.";

  NList il(instance);
  correct = false;
  if (4 == il.length())
  {
    NList onl = il.first();  //object name
    NList nll = il.second(); //node list
    NList fml = il.third();  //fileloc list
    NList dpl = il.fourth(); //duplicate times

    //Check Object Type
    string objName = "";
    ListExpr objType;
    if (onl.isString())
    {
      correct = true;
      objName = onl.str();
      if (0 == objName.length())
      {
        cmsg.inFunError("Object Name cannot be empty.");
      }
      else
      {
        //Read the object from current database
        SecondoCatalog* ctlg = SecondoSystem::GetCatalog();
        if (ctlg->IsSystemObject(objName))
        {
          correct = false;
          cmsg.inFunError("Cannot distribute system object.");
        } else if (ctlg->IsObjectName(objName))
        {
          objType = ctlg->GetObjectTypeExpr(objName);
        } else
        {
/*
If the object doesn't exist in the database,
then its schema file must be kept in master node's parallel directory.
The schema file is further explained in ~ffeed~ and ~fconsume~ operators.

*/
          string filePath = getLocalFilePath("", objName, "_type");
          if (!FileSystem::FileOrFolderExists(filePath))
          {
            correct = false;
            cmsg.inFunError(
                "Object " + objName + " doesn't exist");
          }
          else if (!(nl->ReadFromFile(filePath, objType)))
          {
            correct = false;
            cmsg.inFunError(
                "Incorrect nested list in file " + filePath );
          }
        }
        if (correct && !listutils::isRelDescription(objType))
        {
          correct = false;
          cmsg.inFunError(
              "Expect a relation object. ");
        }
      }
    }

    //Check nodes list
    if (correct)
    {
      //Read the nodeList
      NList nodes = nll;
      while (!nodes.isEmpty())
      {
        NList addr = nodes.first();
        if (!addr.isString())
        {
          correct = false;
          cmsg.inFunError(
              "Expect string list of IP addresses.");
          break;
        }
        nodes.rest();
      }
    }

    //Check the type of fileLocList
    if (correct)
    {
      bool isOK = false;
      NList rows = fml;
      while (!rows.isEmpty())
      {
        NList aRow = rows.first();
        if (2 == aRow.length())
        {
          if (aRow.first().isInt())
          {
            NList CFs = aRow.second();
            while(!CFs.isEmpty())
            {
              NList aCF = CFs.first();
              if (aCF.isInt())
                isOK = true;
              else
                isOK = false;

              if (!isOK)
                break;
              CFs.rest();
            }
          }
          else
            isOK = false;
        }
        else
          isOK = false;

        if (!isOK)
          break;
        rows.rest();
      }

      if (!isOK)
      {
        correct = false;
        cmsg.inFunError(flocErr);
      }
    }

    //Check the duplicate times
    size_t dpTime = 0;
    if (correct)
    {
      if (dpl.isInt())
      {
        dpTime = dpl.intval();
        if (dpTime < 1)
          correct = false;
      }
      else
        correct = false;

      if (!correct)
        cmsg.inFunError(
            "Expect a positive integer duplicate times value.");
    }

    if (correct)
    {
      fList *FLL = new fList(objName, NList(objType),
          nll, fml, dpTime, true);
      if(FLL->isOK())
        return SetWord(FLL);
      else
        correct = false;
    }
  }
  else
    cmsg.inFunError(typeErr);

  assert(!correct);
  return SetWord(Address(0));
}

ListExpr fList::Out(ListExpr typeInfo, Word value)
{
  if (value.addr)
  {
    fList* fl = static_cast<fList*>(value.addr);

    return nl->FiveElemList(
        NList(fl->getObjName(), true, false).listExpr(),
        fl->getTypeList().listExpr(),
        fl->getNodeList().listExpr(),
        fl->getLocList().listExpr(),
        NList(fl->getDupTimes()).listExpr() );
  }
  else
    return nl->SymbolAtom("undef");
}

Word fList::Create(const ListExpr typeInfo)
{
  cerr << "In Create" << endl;
  return SetWord(new fList("", NList(), NList(), NList(), 1));
}

void fList::Delete(const ListExpr typeInfo, Word& w)
{
  cerr << "In Delete" << endl;
  delete (fList*)w.addr;
  w.addr = 0;
}

void fList::Close(const ListExpr typeInfo, Word& w)
{
  cerr << "In Close" << endl;
  delete (fList*)w.addr;
  w.addr = 0;
}


Word fList::Clone(const ListExpr typeInfo, const Word& w)
{
  cerr << "In Clone" << endl;
  return SetWord(new fList(*(fList*)w.addr));
}

bool fList::Save(SmiRecord& valueRecord, size_t& offset,
                 const ListExpr typeInfo, Word& w)
{
  bool ok = true;

  ListExpr valueList = Out(typeInfo, w);
  valueList = nl->OneElemList(valueList);
  string valueStr;
  nl->WriteToString(valueStr, valueList);
  int valueLength = valueStr.length();
  ok = ok && valueRecord.Write(&valueLength, sizeof(int), offset);
  offset += sizeof(int);
  ok = ok && valueRecord.Write(valueStr.data(), valueLength, offset);
  offset += valueLength;

  return ok;
}

bool fList::Open(SmiRecord& valueRecord,
                 size_t& offset,
                 const ListExpr typeInfo,
                 Word& value)
{
  int valueLength;
  string valueStr = "";
  ListExpr valueList = 0;
  char *buf = 0;
  ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ERRORS"));
  bool correct;

  bool ok = true;
  ok = ok && valueRecord.Read(&valueLength, sizeof(int), offset);
  offset += sizeof(int);
  buf = new char[valueLength];
  ok = ok && valueRecord.Read(buf, valueLength, offset);
  offset += valueLength;
  valueStr.assign(buf, valueLength);
  delete []buf;
  nl->ReadFromString(valueStr, valueList);

  value = RestoreFromList(typeInfo, nl->First(valueList),
      1, errorInfo, correct);

  if (errorInfo != 0)
    nl->Destroy(errorInfo);
  nl->Destroy(valueList);
  return ok;
}

Word fList::RestoreFromList(
    const ListExpr typeInfo, const ListExpr instance,
    const int errorPos, ListExpr& errorInfo, bool& correct )
{
  NList il = NList(instance);
  string objName = il.first().str();
  NList typeList = il.second();
  NList nodeList = il.third();
  NList locList = il.fourth();
  size_t dupTimes = il.fifth().intval();

  fList* fl = new fList(objName, typeList, nodeList,
      locList, dupTimes, true);
  correct = fl->isOK();
  if (correct)
    return SetWord(fl);
  else
    return SetWord(Address(0));

}

/*
As in ~In~ function, the type of the fileLocList is already checked,
it's not necessary to check the type here again.
However, it's still necessary to check whether the value of the list
is available.
During the check period, following conditions are required:

  * row number of the matrix is limited in [ 1 .. nodesNum ]

  * column number is larger than 1

  * duplicate number is limited in [ 1 .. nodesNum ]

  * a cell file can be duplicated on each slave at most once.

*/
void fList::verifyLocList()
{
  if (fileLocList.isEmpty())
    isAvailable = false;
  else
  {
    mrNum = 0;
    mcNum = -1;

    NList fll = fileLocList;
    while (!fll.isEmpty())
    {
      NList aRow = fll.first();
      mrNum++;
      int nodeNum = aRow.first().intval();
      if (nodeNum > getNodesNum())
      {
        cerr << "Improper matrix row number : "
            << nodeNum << endl;
        isAvailable = false;
        break;
      }
      NList cfList = aRow.second();
      while (!cfList.isEmpty())
      {
        NList aCF = cfList.first();
        int cellNum = aCF.intval();
        if (cellNum < 1)
        {
          cerr << "Negative matrix column number : "
              << cellNum << endl;
          isAvailable = false;
          break;
        }
        mcNum = (cellNum > mcNum) ? cellNum : mcNum;

        if (!isAvailable)
          break;
        cfList.rest();
      }
      if (!isAvailable)
        break;
      fll.rest();
    }
  }
}

struct fListInfo: ConstructorInfo
{
  fListInfo()
  {
    name = "flist";
    signature = "-> " + Kind::SIMPLE();
    typeExample = "flist";
    listRep = "(<objName> <nodeList> <fileLocList><dupTimes>)";
    valueExample =
        "(\"plz\" (\"10.10.10.10\" \"10.10.10.11\")"
        " ( ( (1 (1 2)) (2 (1 2)) ) ) 2) ";
    remarks = "";
  }
};
struct fListFunctions: ConstructorFunctions<fList>
{
  fListFunctions()
  {
    in = fList::In;
    out = fList::Out;

    create = fList::Create;
    deletion = fList::Delete;
    close = fList::Close;
    clone = fList::Clone;
    kindCheck = fList::CheckFList;

    restoreFromList = fList::RestoreFromList;
    save = fList::Save;
    open = fList::Open;
  }
};

fListInfo fli;
fListFunctions flf;
TypeConstructor flTC(fli, flf);

/*
6 Auxiliary functions

*/
string tranStr(const string& s,
                 const string& from, const string& to)
{
  string result = s;

  size_t fLen = from.length();
  size_t tLen = to.length();
  size_t end = s.size();
  size_t p1 = 0;
  size_t p2 = 0;

  while (p1 < end)
  {
    p2 = result.find_first_of(from, p1);

    if ( p2 != string::npos)
    {
      result.replace(p2, fLen, to);
      p1 = p2 + tLen;
    }
    else
      p1 = end;
  }

  return result;
}

/*
The ~getFilePath~ function is used to set the path of the type and
data files produced by ~fconsume~, ~ffeed~ and ~fdistribute~ operators.

If a specified file path is not given, then it reads the
~SecondoFilePath~ variable set in the SecondoConfig.ini that is
denoted by SECONDO\_CONFIG parameter.
And the path must be an absoloute path.
By default, the path will be set to SECONDO\_BUILD\_DIR/bin/parallel

If an non-default path is unavailable or not exist,
then a warning message will be given.

*/
string getLocalFilePath(string filePath,
                   const string filePrefix,
                   string fSfx,
                   bool extendPath)
{

  bool pathOK = false, alarm = false;
  string path = "";
  int cdd = 0;
  while (!pathOK && cdd < 3)
  {
    if (0 == cdd) {
      path = filePath;
    }
    else if (1 == cdd) {
      path = SmiProfile::GetParameter("ParallelSecondo",
          "SecondoFilePath","", string(getenv("SECONDO_CONFIG")));
    }
    else {
      path = FileSystem::GetCurrentFolder();
      FileSystem::AppendItem(path, "parallel");
    }

    if (path.length() > 0)
    {
      if (path.find_last_of("/") == (path.length() - 1))
        path = path.substr(0, path.length() - 1);

      //In case the parent folder doesn't exist.
      if ( FileSystem::IsDirectory(path) ) {
        pathOK = true;
      }
      else {
        pathOK = FileSystem::CreateFolder(path);
        alarm = true;
      }
    }
    cdd++;
  }

  if(pathOK && extendPath){
    FileSystem::AppendItem(path, filePrefix + fSfx);
  }

  // When there is no specific path is given,
  // then no warning messages.
  if (pathOK && alarm)
  {
    cerr << "Warning! The given path is unavailable or not exit, "
        "\n then the path " << path << " is used.\n\n";
  }

  return (pathOK ? path : "");
}


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
    AddTypeConstructor(&flTC);

    AddOperator(doubleExportInfo(),
        doubleExportValueMap, doubleExportTypeMap);
    AddOperator(paraHashJoinInfo(),
        paraHashJoinValueMap, paraHashJoinTypeMap);

    AddOperator(paraJoinInfo(),
        paraJoinValueMap, paraJoinTypeMap);

    AddOperator(paraJoin2Info(),
        paraJoin2ValueMap, paraJoin2TypeMap);

    AddOperator(add0TupleInfo(),
        add0TupleValueMap, add0TupleTypeMap);


    AddOperator(TUPSTREAMInfo(), 0, TUPSTREAMType);
    AddOperator(TUPSTREAM2Info(), 0, TUPSTREAM2Type);
    AddOperator(TUPSTREAM3Info(), 0, TUPSTREAM3Type);

    AddOperator(&fconsumeOp);
    fconsumeOp.SetUsesArgsInTypeMapping();
    AddOperator(&ffeedOp);
    ffeedOp.SetUsesArgsInTypeMapping();

    AddOperator(&hadoopjoinOp);
    hadoopjoinOp.SetUsesArgsInTypeMapping();

    AddOperator(&fdistributeOp);
    fdistributeOp.SetUsesArgsInTypeMapping();

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

