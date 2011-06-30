/*
This file is part of SECONDO.

Copyright (C) 2011, University in Hagen, Department of Computer Science,
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

2011, May Simone Jandt

*/

#include "JListTID.h"
#include "../../include/ListUtils.h"
#include "../../include/NestedList.h"
#include "../../include/Symbols.h"

static bool DEBUG = false;
/*

1 Implementation of class ~JTupleIdList~

1.1. Constructors and deconstructors

*/


JListTID::JListTID():Attribute()
{
  if(DEBUG) cout << "JListTID::jListTID()" << endl;
}

JListTID::JListTID(bool defined): Attribute(defined), elemlist(0)
{
  if(DEBUG) cout << "JListTID::jListTID(bool)" << endl;
}

JListTID::JListTID(const JListTID& other) :
  Attribute(other.IsDefined()), elemlist(other.GetList())
{
  if(DEBUG) cout << "JListTID::JListTID(JListTID)" << endl;
}

JListTID::JListTID(const TupleIdentifier& inId) :  Attribute(true), elemlist(0)
{
  if(DEBUG) cout << "JListTID::jListTID(TupleIdentifier)" << endl;
  elemlist.Append(inId);
}

JListTID::~JListTID()
{}

/*
1.2 Getter and Setter for private Attributes

*/

DbArray<TupleIdentifier> JListTID::GetList() const
{
  if(DEBUG) cout << "JListTID::getlist" << endl;
  return elemlist;
}

void JListTID::SetList(const DbArray<TupleIdentifier> inList)
{
  if(DEBUG) cout << "JListTID::setlist" << endl;
  elemlist.copyFrom(inList);
}

/*
1.3 Override Methods from Attribute

*/

void JListTID::CopyFrom(const Attribute* right)
{
  if(DEBUG) cout << "JListTID::copyfrom" << endl;
  SetDefined(right->IsDefined());
  if (right->IsDefined())
  {
    JListTID* source = (JListTID*) right;
    elemlist.copyFrom(source->GetList());
  }
}

Attribute::StorageType JListTID::GetStorageType() const
{
  return Default;
}

size_t JListTID::HashValue() const
{
  if(DEBUG) cout << "JListTID::hashvalue" << endl;
  size_t result = 0;
  if (IsDefined())
  {
    TupleIdentifier t;
    for (int i = 0; i < elemlist.Size(); i++)
    {
      elemlist.Get(i,t);
      result += t.HashValue();
    }
  }
  return result;
}

Attribute* JListTID::Clone() const
{
  if(DEBUG) cout << "JListTID::clone" << endl;
  return new JListTID(*this);
}


bool JListTID::Adjacent(const Attribute* attrib) const
{if(DEBUG) cout << "JListTID::adjacent" << endl;
  return false;
}


int JListTID::Compare(const Attribute* rhs) const
{
  if(DEBUG) cout << "JListTID::compare attr" << endl;
  JListTID* in = (JListTID*) rhs;
  return Compare(*in);
}

int JListTID::Compare(const JListTID& in) const
{
  if(DEBUG) cout << "JListTID::compare" << endl;
  if (!IsDefined() && !in.IsDefined()) return 0;
  else
  {
    if (IsDefined() && !in.IsDefined()) return 1;
    else
    {
      if (!IsDefined() && in.IsDefined()) return -1;
      else
      {
        if (elemlist.Size() < in.elemlist.Size()) return -1;
        else
        {
          if (elemlist.Size() > in.elemlist.Size()) return 1;
          else
          {
            int res = 0;
            TupleIdentifier t1, t2;
            for (int i = 0; i < elemlist.Size(); i++)
            {
              elemlist.Get(i,t1);
              in.elemlist.Get(i,t2);
              res = t1.Compare(t2);
              if (res != 0) return res;
            }
            return 0;
          }
        }
      }
    }
  }
}

int JListTID::NumOfFLOBs() const
{
  if(DEBUG) cout << "JListTID::numofflobs" << endl;
  return 1;
}

Flob* JListTID::GetFLOB(const int n)
{
  if(DEBUG)
  {
    cout << "JListTID::getflob: " << n << endl;
    this->Print(cout);
  }
  if (n == 0) return &elemlist;
  else return 0;
}

void JListTID::Destroy()
{
  if(DEBUG) cout << "JListTID::destroy" << endl;
  elemlist.Destroy();
}

size_t JListTID::Sizeof() const
{
  if(DEBUG) cout << "JListTID::sizeof" << endl;
  return sizeof(JListTID);
}

ostream& JListTID::Print(ostream& os) const
{
  if (DEBUG) cout << "JListTID::Print" << endl;
  os << "List of tuple ids: " << endl;
  if (IsDefined())
  {
    TupleIdentifier t;
    for(int i = 0; i < elemlist.Size(); i++)
    {
      os << i+1 << ".TupleId: " ;
      elemlist.Get(i,t);
      t.Print(os);
      os << endl;
    }
    os << "end of tuple id list.";
  }
  else
  {
    os << Symbol::UNDEFINED();
  }
  os << endl;
  return os;
}

const string JListTID::BasicType()
{
  return "jlisttid";
}

const bool JListTID::checkType(const ListExpr type)
{
  return listutils::isSymbol(type, BasicType());
}

/*
1.4 Standard Methods

*/


JListTID& JListTID::operator=(const JListTID& other)
{
  if(DEBUG) cout << "JListTID::operator=" << endl;
  SetDefined(other.IsDefined());
  if (other.IsDefined())
  {
    elemlist.copyFrom(other.elemlist);
  }
  return *this;
}

bool JListTID::operator==(const JListTID& other) const
{
  if(DEBUG) cout << "JListTID::operator==" << endl;
  if (Compare(other) == 0) return true;
  else return false;
}

/*
1.5 Operators for Secondo Integration

*/

ListExpr JListTID::Out(ListExpr typeInfo, Word value)
{
  if (DEBUG) cout << "JListTID::Out" << endl;
  JListTID* source = (JListTID*) value.addr;
  if (DEBUG) source->Print(cout);
  if (source->IsDefined())
  {
    if(source->elemlist.Size() == 0) return nl->TheEmptyList();
    else
    {
      NList result(nl->TheEmptyList());
      TupleIdentifier e;
      bool first = true;
      for (int i = 0; i < source->elemlist.Size(); i++)
      {
        source->elemlist.Get(i,e);
        Word wt = SetWord(&e);
        NList tl(TupleIdentifier::Out(nl->TheEmptyList(), wt));
        if (first)
        {
          result = tl.enclose().enclose();
          first = false;
        }
        else
        {
          result.append(tl.enclose());
        }
      }
      return result.listExpr();
    }
  }
  else
  {
    return nl->SymbolAtom(Symbol::UNDEFINED());
  }
}


Word JListTID::In(const ListExpr typeInfo, const ListExpr instance,
               const int errorPos, ListExpr& errorInfo, bool& correct)
{
  if (DEBUG)
  {
    cout << "JListTID::in" << endl;
    NList inst(instance);
    inst.writeAsStringTo(cout);
  }
  if(nl->IsEqual(instance,Symbol::UNDEFINED()))
  {
    correct=true;
    return SetWord(Address(new JListTID(false)));
  }

  ListExpr rest = instance;
  ListExpr first = nl->TheEmptyList();
  correct = true;
  JListTID* in = new JListTID(true);
  while( !nl->IsEmpty( rest ) )
  {
    first = nl->First( rest );
    rest = nl->Rest( rest );
    Word wt = TupleIdentifier::In(nl->TheEmptyList(), nl->First(first),
                                  errorPos, errorInfo, correct);
    if (correct)
    {
      TupleIdentifier* t = (TupleIdentifier*) wt.addr;
      in->Append(*t);
      t->DeleteIfAllowed();
      t = 0;
    }
    else
    {

      nl->Append(errorInfo,
                 nl->StringAtom("Incorrect TupleIdentifier in list."));
      in->DeleteIfAllowed();
      delete in;
      return SetWord(Address(0));
    }
  }
  if (DEBUG) in->Print(cout);
  return SetWord(in);
}

Word JListTID::Create(const ListExpr typeInfo)
{
  if(DEBUG) cout << "JListTID::create" << endl;
  return SetWord(new JListTID(true));
}


void JListTID::Delete( const ListExpr typeInfo, Word& w )
{
  if(DEBUG) cout << "JListTID::delete" << endl;
 JListTID* obj = (JListTID*) w.addr;
 obj->DeleteIfAllowed();
 w.addr = 0;
}


void JListTID::Close( const ListExpr typeInfo, Word& w )
{
  if(DEBUG) cout << "JListTID::close" << endl;
  ((JListTID*) w.addr)->DeleteIfAllowed();
  w.addr = 0;
}


Word JListTID::Clone( const ListExpr typeInfo, const Word& w )
{

  if(DEBUG) cout << "JListTID::clone" << endl;
  return new JListTID(*((JListTID*) w.addr));
}


void* JListTID::Cast( void* addr )
{
  if(DEBUG) cout << "JListTID::cast" << endl;
  return (new (addr) JListTID);
}


bool JListTID::KindCheck( ListExpr type, ListExpr& errorInfo )
{
  if(DEBUG) cout << "JListTID::kindcheck" << endl;
  return checkType(type);
}



int JListTID::SizeOf()
{
  if(DEBUG) cout << "JListTID::sizeof" << endl;
  return sizeof(JListTID);
}


bool JListTID::Save(SmiRecord& valueRecord, size_t& offset,
                    const ListExpr typeInfo, Word& value )
{
  if (DEBUG) cout << "JListTID::save" << endl;
  JListTID* obj = (JListTID*) value.addr;
  if (DEBUG) obj->Print(cout);
  for( int i = 0; i < obj->NumOfFLOBs(); i++ )
  {
    Flob *tmpFlob = obj->GetFLOB(i);
    SecondoCatalog* ctlg = SecondoSystem::GetCatalog();
    SmiRecordFile* rf = ctlg->GetFlobFile();
    //cerr << "FlobFileId = " << fileId << endl;
    tmpFlob->saveToFile(rf, *tmpFlob);
  }
  valueRecord.Write( obj, obj->SizeOf(), offset );
  offset += obj->SizeOf();
  return true;
}


bool JListTID::Open(SmiRecord& valueRecord, size_t& offset,
                    const ListExpr typeInfo, Word& value )
{
  if (DEBUG) cout << "JListTID::open" << endl;
  JListTID* obj = new JListTID (true);
  valueRecord.Read( obj, obj->SizeOf(), offset );
  obj->del.refs = 1;
  obj->del.SetDelete();
  offset += obj->SizeOf();
  value = SetWord( obj );
  if (DEBUG) obj->Print(cout);
  return true;
}

/*
1.6 Helpful Operators

*/

void JListTID::Append (const TupleIdentifier& e)
{
  if(DEBUG)
  {
    cout << "JListTID::Append" << endl;
    e.Print(cout);
  }
  elemlist.Append(e);
}

int JListTID::GetNoOfComponents() const
{
  if(DEBUG)
    cout << "JListTID::GetNoComponents: " << elemlist.Size() << endl;
  return elemlist.Size();
}

void JListTID::Get(const int i, TupleIdentifier& e) const
{
  elemlist.Get(i,e);
}

void JListTID::Put(const int i, const TupleIdentifier& e)
{
  elemlist.Put(i,e);
}
