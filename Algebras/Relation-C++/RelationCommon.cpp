/*
//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]

[1] Implementation of the Common Relation Algebra

March 2003 Victor Almeida created the new Relational Algebra organization.

[TOC]

1 Overview

The Relational Algebra basically implements two type constructors, namely ~tuple~ and ~rel~.
More information about the Relational Algebra can be found in the RelationAlgebra.h header
file.

Some functionalities are the same for both the Main Memory and the Persistent Relational
Algebra. These common functionalities belongs to this implementation file.

2 Defines, includes, and constants

*/
#include "RelationAlgebra.h"
#include "SecondoSystem.h"
#include <set>

extern NestedList *nl;

/*
3 Implementation of the class ~TupleType~

A ~TupleType~ is a collection (an array) of all attribute types (~AttributeType~)
of the tuple. This structure contains the metadata of a tuple attributes.

*/
TupleType::TupleType( const ListExpr typeInfo ):
  noAttributes( nl->ListLength( nl->Second( typeInfo ) ) ),
  attrTypeArray( new AttributeType[noAttributes] ),
  totalSize( 0 )
{
  static AlgebraManager* algM = SecondoSystem::GetAlgebraManager();
  ListExpr rest = nl->Second( typeInfo );
  int i = 0;

  while( !nl->IsEmpty( rest ) )
  {
    ListExpr first = nl->First( rest );
    rest = nl->Rest( rest );

    int algId = nl->IntValue( nl->First( nl->Second( first ) ) ),
        typeId = nl->IntValue( nl->Second( nl->Second( first ) ) ),
        size = (algM->SizeOfObj(algId, typeId))();

    totalSize += size;
    attrTypeArray[i++] = AttributeType( algId, typeId, size );
  }
  assert( i == noAttributes );
}

TupleType::TupleType( const TupleType& tupleType ):
  noAttributes( tupleType.GetNoAttributes() ),
  attrTypeArray( new AttributeType[tupleType.GetNoAttributes()] ),
  totalSize( 0 )
{
  for( int i = 0; i < noAttributes; i++ )
  {
    attrTypeArray[i] = tupleType.GetAttributeType( i );
    totalSize += attrTypeArray[i].size;
  }
}

TupleType::TupleType( const int noAttrs, AttributeType *attrs ):
  noAttributes( noAttrs ),
  attrTypeArray( attrs ),
  totalSize( 0 )
{
  for( int i = 0; i < noAttrs; i++ )
  {
    totalSize += attrs[i].size;
  }
}

TupleType::~TupleType()
{
  delete []attrTypeArray;
}

const int TupleType::GetNoAttributes() const
{
  return noAttributes;
}

const int TupleType::GetTotalSize() const
{
  return totalSize;
}

const AttributeType& TupleType::GetAttributeType( const int index ) const
{
  assert( index >= 0 && index < noAttributes );
  return attrTypeArray[index];
}

void TupleType::PutAttributeType( const int index, const AttributeType& attrType )
{
  assert( index >= 0 && index < noAttributes );
  attrTypeArray[index] = attrType;
}

/*
4 Implementation of the class ~Tuple~

*/
Tuple *Tuple::In( ListExpr typeInfo, ListExpr value, int errorPos, ListExpr& errorInfo, bool& correct )
{
  int  attrno, algebraId, typeId, noOfAttrs;
  Word attr;
  Tuple* tupleaddr;
  bool valueCorrect;
  ListExpr first, firstvalue, valuelist, attrlist;

  attrno = 0;
  noOfAttrs = 0;
  tupleaddr = new Tuple( nl->First( typeInfo ) );

  attrlist =  nl->Second(nl->First(typeInfo));
  valuelist = value;
  correct = true;
  if (nl->IsAtom(valuelist))
  {
    correct = false;

    cout << "Error in reading tuple: an atom instead of a list of values." << endl;
    cout << "Tuple no." << errorPos << endl;
    cout << "The tuple is: " << endl;
    nl->WriteListExpr(value);

    errorInfo = nl->Append(errorInfo,
      nl->FourElemList(nl->IntAtom(71), nl->SymbolAtom("tuple"), nl->IntAtom(1),
      nl->IntAtom(errorPos)));
    delete tupleaddr;
    return 0;
  }
  else
  {
    AlgebraManager* algM = SecondoSystem::GetAlgebraManager();
    while (!nl->IsEmpty(attrlist))
    {
      first = nl->First(attrlist);
      attrlist = nl->Rest(attrlist);
      attrno++;
      algebraId = nl->IntValue(nl->First(nl->Second(first)));
      typeId = nl->IntValue(nl->Second(nl->Second(first)));
      if (nl->IsEmpty(valuelist))
      {
        correct = false;

        cout << "Error in reading tuple: list of values is empty." << endl;
        cout << "Tuple no." << errorPos << endl;
        cout << "The tuple is: " << endl;
        nl->WriteListExpr(value);

        errorInfo = nl->Append(errorInfo,
          nl->FourElemList(nl->IntAtom(71), nl->SymbolAtom("tuple"), nl->IntAtom(2),
            nl->IntAtom(errorPos)));
        delete tupleaddr;
        return 0;
      }
      else
      {
        firstvalue = nl->First(valuelist);
        valuelist = nl->Rest(valuelist);
        attr = (algM->InObj(algebraId, typeId))(nl->Rest(first),
                 firstvalue, attrno, errorInfo, valueCorrect);
        if (valueCorrect)
        {
          correct = true;
          tupleaddr->PutAttribute(attrno - 1, (Attribute *)attr.addr);
          noOfAttrs++;
        }
        else
        {
          correct = false;

          cout << "Error in reading tuple: wrong attribute value representation." << endl;
          cout << "Tuple no." << errorPos << endl;
          cout << "The tuple is: " << endl;
          nl->WriteListExpr(value);
          cout << endl << "The attribute is: " << endl;
          nl->WriteListExpr(firstvalue);
          cout << endl;

          errorInfo = nl->Append(errorInfo,
           nl->FiveElemList(nl->IntAtom(71), nl->SymbolAtom("tuple"), nl->IntAtom(3),
          nl->IntAtom(errorPos), nl->IntAtom(attrno)));
          delete tupleaddr;
          return 0;
        }
      }
    }
    if (!nl->IsEmpty(valuelist))
    {
      correct = false;

      cout << "Error in reading tuple: too many attribute values." << endl;
      cout << "Tuple no." << errorPos << endl;
      cout << "The tuple is: " << endl;
      nl->WriteListExpr(value);

      errorInfo = nl->Append(errorInfo,
      nl->FourElemList(nl->IntAtom(71), nl->SymbolAtom("tuple"), nl->IntAtom(4),
      nl->IntAtom(errorPos)));
      delete tupleaddr;
      return 0;
    }
  }
  assert( tupleaddr->GetNoAttributes() == noOfAttrs );
  return tupleaddr;
}

ListExpr Tuple::Out( ListExpr typeInfo )
{
  int attrno, algebraId, typeId;
  ListExpr l, lastElem, attrlist, first, valuelist;

  AlgebraManager* algM = SecondoSystem::GetAlgebraManager();
  attrlist = nl->Second(nl->First(typeInfo));
  attrno = 0;
  l = nl->TheEmptyList();
  while (!nl->IsEmpty(attrlist))
  {
    first = nl->First(attrlist);
    attrlist = nl->Rest(attrlist);
    algebraId = nl->IntValue(nl->First(nl->Second(first)));
    typeId = nl->IntValue(nl->Second(nl->Second(first)));

    Attribute *attr = GetAttribute( attrno );
    valuelist = (algM->OutObj(algebraId, typeId))(nl->Rest(first), SetWord(attr));
    attrno++;
    if (l == nl->TheEmptyList())
    {
      l = nl->Cons(valuelist, nl->TheEmptyList());
      lastElem = l;
    }
    else
      lastElem = nl->Append(lastElem, valuelist);
  }
  return l;
}

/*
5 Implementation of the class ~Relation~

*/
Relation *Relation::In( ListExpr typeInfo, ListExpr value, int errorPos, ListExpr& errorInfo, bool& correct )
{
  ListExpr tuplelist, TupleTypeInfo, first;
  Relation* rel;
  Tuple* tupleaddr;
  int tupleno, count;
  bool tupleCorrect;

  correct = true;
  count = 0;

  rel = new Relation( typeInfo );

  tuplelist = value;
  TupleTypeInfo = nl->TwoElemList(nl->Second(typeInfo),
    nl->IntAtom(nl->ListLength(nl->Second(nl->Second(typeInfo)))));
  tupleno = 0;
  if (nl->IsAtom(value))
  {
    correct = false;
    errorInfo = nl->Append(errorInfo,
    nl->ThreeElemList(nl->IntAtom(70), nl->SymbolAtom("rel"), tuplelist));
    return rel;
  }
  else
  { // increase tupleno
    while (!nl->IsEmpty(tuplelist))
    {
      first = nl->First(tuplelist);
      tuplelist = nl->Rest(tuplelist);
      tupleno++;
      tupleaddr = Tuple::In(TupleTypeInfo, first, tupleno, errorInfo, tupleCorrect);

      if (tupleCorrect)
      {
        assert( tupleaddr->IsFree() == false );

        rel->AppendTuple(tupleaddr);
        count++;
      }
      else
      {
        correct = false;
      }
    }
    if (!correct)
    {
      errorInfo = nl->Append(errorInfo,
      nl->TwoElemList(nl->IntAtom(72), nl->SymbolAtom("rel")));
    }
    assert( rel->GetNoTuples() == count );
    return rel;
  }
}

ListExpr Relation::Out( ListExpr typeInfo )
{
  Tuple* t;
  ListExpr l, lastElem, tlist, TupleTypeInfo;

  RelationIterator* rit = MakeScan();
  l = nl->TheEmptyList();

  //cerr << "OutRel " << endl;
  while ( (t = rit->GetNextTuple()) != 0 )
  {
    TupleTypeInfo = nl->TwoElemList(nl->Second(typeInfo),
          nl->IntAtom(nl->ListLength(nl->Second(nl->Second(typeInfo)))));
    tlist = t->Out(TupleTypeInfo);
    if (l == nl->TheEmptyList())
    {
      l = nl->Cons(tlist, nl->TheEmptyList());
      lastElem = l;
    }
    else
      lastElem = nl->Append(lastElem, tlist);
  }
  delete rit;
  return l;
}

/*
6 Auxilary Functions

6.1 Function ~TypeOfRelAlgSymbol~

Transforms a list expression ~symbol~ into one of the values of
type ~RelationType~. ~Symbol~ is allowed to be any list. If it is not one
of these symbols, then the value ~error~ is returned.

*/
RelationType TypeOfRelAlgSymbol (ListExpr symbol)
{
  string s;

  if (nl->AtomType(symbol) == SymbolType)
  {
    s = nl->SymbolValue(symbol);
    if (s == "rel"   ) return rel;
    if (s == "tuple" ) return tuple;
    if (s == "stream") return stream;
    if (s == "map"   ) return ccmap;
    if (s == "bool"  ) return ccbool;
  }
  return error;
}

/*
6.2 Function ~FindAttribute~

Here ~list~ should be a list of pairs of the form (~name~,~datatype~).
The function ~FindAttribute~ determines whether ~attrname~ occurs as one of
the names in this list. If so, the index in the list (counting from 1)
is returned and the corresponding datatype is returned in ~attrtype~.
Otherwise 0 is returned. Used in operator ~attr~, for example.

*/
int FindAttribute( ListExpr list, string attrname, ListExpr& attrtype)
{
  ListExpr first, rest;
  int j;
  string  name;
 
  if (nl->IsAtom(list))
    return 0;
  rest = list;
  j = 1;
  while (!nl->IsEmpty(rest))
  {
    first = nl->First(rest);
    rest = nl->Rest(rest);
    if ((nl->ListLength(first) == 2) &&
       (nl->AtomType(nl->First(first)) == SymbolType))
    {
      name = nl->SymbolValue(nl->First(first));
      if (name == attrname)
      {
        attrtype = nl->Second(first);
        return j;
      }
    }
    else
      return 0; // typeerror
    j++;
  }
  return 0; // attrname not found
}

/*
6.3 Function ~ConcatLists~

Concatenates two lists.

*/
ListExpr ConcatLists( ListExpr list1, ListExpr list2)
{
  if (nl->IsEmpty(list1))
  {
    return list2;
  }
  else
  {
    return nl->Cons(nl->First(list1), ConcatLists(nl->Rest(list1), list2));
  }
}

/*
6.4 Function ~AttributesAreDisjoint~

Checks wether two ListExpressions are of the form
((a1 t1) ... (ai ti)) and ((b1 d1) ... (bj dj))
and wether the ai and the bi are disjoint.

*/
bool AttributesAreDisjoint(ListExpr a, ListExpr b)
{
  set<string> aNames;
  ListExpr rest = a;
  ListExpr current;

  while(!nl->IsEmpty(rest))
  {
    current = nl->First(rest);
    rest = nl->Rest(rest);
    if((nl->ListLength(current) == 2)
      && (nl->IsAtom(nl->First(current)))
      && (nl->AtomType(nl->First(current)) == SymbolType)
      && (nl->IsAtom(nl->Second(current)))
      && (nl->AtomType(nl->Second(current)) == SymbolType))
    {
      aNames.insert(nl->SymbolValue(nl->First(current)));
    }
    else
    {
      return false;
    }
  }
  rest = b;
  while(!nl->IsEmpty(rest))
  {
    ListExpr current = nl->First(rest);
    rest = nl->Rest(rest);
    if((nl->ListLength(current) == 2)
      && (nl->IsAtom(nl->First(current)))
      && (nl->AtomType(nl->First(current)) == SymbolType)
      && (nl->IsAtom(nl->Second(current)))
      && (nl->AtomType(nl->Second(current)) == SymbolType))
    {
      if(aNames.find(nl->SymbolValue(nl->First(current))) != aNames.end())
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  return true;
}

/*
6.5 Function ~Concat~

Copies the attribute values of two tuples
(words) ~r~ and ~s~ into tuple (word) ~t~.

*/
void Concat (Word r, Word s, Word& t)
{
  int rnoattrs, snoattrs, tnoattrs;
  Attribute* attr;

  rnoattrs = ((Tuple*)r.addr)->GetNoAttributes();
  snoattrs = ((Tuple*)s.addr)->GetNoAttributes();
  tnoattrs = rnoattrs + snoattrs;

//VTA
  assert( ((Tuple*)t.addr)->GetNoAttributes() == tnoattrs );
//  ((Tuple*)t.addr)->SetNoAttributes(tnoattrs);

  for( int i = 0; i < rnoattrs; i++)
  {
    attr = ((Tuple*)r.addr)->GetAttribute(i);
    ((Tuple*)t.addr)->PutAttribute((i), ((StandardAttribute*)attr)->Clone());
  }
  for (int j = rnoattrs; j < tnoattrs; j++)
  {
    attr = ((Tuple*)s.addr)->GetAttribute(j - rnoattrs);
    ((Tuple*)t.addr)->PutAttribute((j), ((StandardAttribute*)attr)->Clone());
  }
}

/*
6.6 Function ~CompareNames~

*/
bool CompareNames(ListExpr list)
{
  vector<string> attrnamestrlist;
  vector<string>::iterator it;
  ListExpr attrnamelist;
  int unique;
  string attrname;

  attrnamelist = list;
  attrnamestrlist.resize(nl->ListLength(list));
  it = attrnamestrlist.begin();
  while (!nl->IsEmpty(attrnamelist))
  {
    attrname = nl->SymbolValue(nl->First(nl->First(attrnamelist)));
    attrnamelist = nl->Rest(attrnamelist);
    unique = std::count(attrnamestrlist.begin(), attrnamestrlist.end(),
                               attrname);
    *it =  attrname;
    if (unique) return false;
    it++;
  }
  return true;
}

/*
6.7 Function ~IsTupleDescription~

Checks wether a ListExpression is of the form
((a1 t1) ... (ai ti)).

*/
bool IsTupleDescription( ListExpr a )
{
  ListExpr rest = a;
  ListExpr current;

  while(!nl->IsEmpty(rest))
  {
    current = nl->First(rest);
    rest = nl->Rest(rest);
    if((nl->ListLength(current) == 2)
      && (nl->IsAtom(nl->First(current)))
      && (nl->AtomType(nl->First(current)) == SymbolType)
      && (nl->IsAtom(nl->Second(current)))
      && (nl->AtomType(nl->Second(current)) == SymbolType))
    {
    }
    else
    {
      return false;
    }
  }
  return true;
}

