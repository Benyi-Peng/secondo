/*
----
This file is part of SECONDO.

Copyright (C) 2015,
Faculty of Mathematics and Computer Science,
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

*/

//
// Created by gstancul on 14.06.19.
//

#include "Attribute.h" // implementation of attribute types
#include "Algebra.h" // definition of the algebra
#include "NestedList.h" // required at many places
#include "QueryProcessor.h" // needed for implementing value mappings
#include "AlgebraManager.h" // e.g., check for a certain kind
#include "Operator.h" // for operator creation
#include "StandardTypes.h" // priovides int, real, string, bool type
#include "Algebras/FText/FTextAlgebra.h"
#include "Symbols.h" // predefined strings
#include "ListUtils.h" // useful functions for nested lists
#include "Algebras/Stream/Stream.h" // wrapper for secondo streams

#include "GenericTC.h" // use of generic type constructors

#include "LogMsg.h" // send error messages

#include "Tools/Flob/DbArray.h" // use of DbArrays

#include "Algebras/Relation-C++/RelationAlgebra.h" // use of tuples

#include "FileSystem.h" // deletion of files

#include <math.h> // required for some operators
#include <stack>
#include <limits>

extern NestedList *nl;
extern QueryProcessor *qp;
extern AlgebraManager *am;

using namespace std;

namespace gstguide {

    class SCircle {
    public:
// constructor doing nothing
        SCircle() {}

// constructor initializing the object
        SCircle(const double _x, const double _y, const double _r) :
                x(_x), y(_y), r(_r) {}

// copy constructor
        SCircle(const SCircle &src) : x(src.x), y(src.y), r(src.r) {}

// assignment operator
        SCircle &operator=(const SCircle &src) {
            x = src.x;
            y = src.y;
            r = src.r;
            return *this;
        }

// destructor
        ~SCircle() {}


        static const string BasicType() { return "scircle"; }

// the checktype function for non-nested types looks always
// the same
        static const bool checkType(const ListExpr list) {
            return listutils::isSymbol(list, BasicType());
        }

        double perimeter() const {
            return 2 * M_PI * r;
        }

        double getX() const { return x; }

        double getY() const { return y; }

        double getR() const { return r; }

    private:
        double x;
        double y;
        double r;


    public:

    };


    ListExpr SCircleProperty() {
        return (nl->TwoElemList(
                nl->FourElemList(
                        nl->StringAtom("Signature"),
                        nl->StringAtom("Example Type List"),
                        nl->StringAtom("List Rep"),
                        nl->StringAtom("Example List")),
                nl->FourElemList(
                        nl->StringAtom("-> SIMPLE"),
                        nl->StringAtom(SCircle::BasicType()),
                        nl->StringAtom("(real real real) = (x,y,r)"),
                        nl->StringAtom("(13.5 -76.0 1.0)")
                )));
    }

    Word InSCircle(const ListExpr typeInfo, const ListExpr instance,
                   const int errorPos, ListExpr &errorInfo, bool &correct) {

        cout << "Hello world GST: Here we are.";
// create a result with addr pointing to 0
        Word res((void *) 0);
// assume an incorrect list
        correct = false;
// check whether the list has three elements
        if (!nl->HasLength(instance, 3)) {
            cmsg.inFunError("expected three numbers");
            return res;
        }
// check whether all elements are numeric
        if (!listutils::isNumeric(nl->First(instance))
            || !listutils::isNumeric(nl->Second(instance))
            || !listutils::isNumeric(nl->Third(instance))) {
            cmsg.inFunError("expected three numbers");
            return res;
        }
// get the numeric values of the elements
        double x = listutils::getNumValue(nl->First(instance));
        double y = listutils::getNumValue(nl->Second(instance));
        double r = listutils::getNumValue(nl->Third(instance));
// check for a valid radius
        if (r <= 0) {
            cmsg.inFunError("invalid radius (<=0)");
            return res;
        }
// list was correct, create the result
        correct = true;
        res.addr = new SCircle(x, y, r);
        return res;

    }

    ListExpr OutSCircle(ListExpr typeInfo, Word value) {
        SCircle *k = (SCircle *) value.addr;
        return nl->ThreeElemList(
                nl->RealAtom(k->getX()),
                nl->RealAtom(k->getY()),
                nl->RealAtom(k->getR()));
    }

    Word CreateSCircle(const ListExpr typeInfo) {
        Word w;
        w.addr = (new SCircle(0, 0, 1.0));
        return w;
    }

    void DeleteSCircle(const ListExpr typeInfo, Word &w) {
        SCircle *k = (SCircle *) w.addr;
        delete k;
        w.addr = 0;
    }

    bool OpenSCircle(SmiRecord &valueRecord,
                     size_t &offset, const ListExpr typeInfo,
                     Word &value) {
        size_t size = sizeof(double);
        double x, y, r;
        bool ok = (valueRecord.Read(&x, size, offset) == size);
        offset += size;
        ok = ok && (valueRecord.Read(&y, size, offset) == size);
        offset += size;
        ok = ok && (valueRecord.Read(&r, size, offset) == size);
        offset += size;
        if (ok) {
            value.addr = new SCircle(x, y, r);
        } else {
            value.addr = 0;
        }
        return ok;
    }

    bool SaveSCircle(SmiRecord &valueRecord, size_t &offset,
                     const ListExpr typeInfo, Word &value) {
        SCircle *k = static_cast <SCircle *>( value.addr );
        size_t size = sizeof(double);
        double v = k->getX();
        bool ok = valueRecord.Write(&v, size, offset);
        offset += size;
        v = k->getY();
        ok = ok && valueRecord.Write(&v, size, offset);
        offset += size;
        v = k->getR();
        ok = ok && valueRecord.Write(&v, size, offset);
        offset += size;
        return ok;
    }

    void CloseSCircle(const ListExpr typeInfo, Word &w) {
        SCircle *k = (SCircle *) w.addr;
        delete k;
        w.addr = 0;
    }

    Word CloneSCircle(const ListExpr typeInfo, const Word &w) {
        SCircle *k = (SCircle *) w.addr;
        Word res;
        res.addr = new SCircle(k->getX(), k->getY(), k->getR());
        return res;
    }

    void *CastSCircle(void *addr) {
        return (new(addr) SCircle);
    }

    bool SCircleTypeCheck(ListExpr type, ListExpr &errorInfo) {
        return nl->IsEqual(type, SCircle::BasicType());
    }

    int SizeOfSCircle() {
        return 3 * sizeof(double);
    }

    TypeConstructor SCircleTC(
            SCircle::BasicType(), // name of the type
            SCircleProperty, // property function
            OutSCircle, InSCircle, // out and in function
            0, 0, // deprecated, don’t think about it
            CreateSCircle, DeleteSCircle, // creation and deletion
            OpenSCircle, SaveSCircle, // open and save functions
            CloseSCircle, CloneSCircle, // close and clone functions
            CastSCircle, // cast function
            SizeOfSCircle, // sizeOf function
            SCircleTypeCheck); // type checking function



    class KafkaAlgebra : public Algebra {
    public:
        KafkaAlgebra() : Algebra() {

            AddTypeConstructor(&SCircleTC);
            SCircleTC.AssociateKind(Kind::SIMPLE());

//        AddTypeConstructor( &ACircleTC );
//        ACircleTC.AssociateKind( Kind::DATA() );
//
//        AddTypeConstructor( &GCircleTC );
//        GCircleTC.AssociateKind( Kind::DATA() );
//
//        AddTypeConstructor( &IntListTC );
//        IntListTC.AssociateKind( Kind::DATA() );
//
//        AddTypeConstructor( &PAVLTC );
//        PAVLTC.AssociateKind( Kind::SIMPLE() );
//
//
//        AddTypeConstructor( &ushortTC);
//        ushortTC.AssociateKind(Kind::DATA());
//
//        AddTypeConstructor( &VStringTC);
//        VStringTC.AssociateKind(Kind::DATA());
//
//
//        AddOperator(&perimeterOp);
//        AddOperator(&distNOp);
//        AddOperator(&countNumberOp);
//        AddOperator(&getCharsOp);
//        AddOperator(&startsWithSOp);
//        AddOperator(&replaceElemOp);
//        AddOperator(&attrIndexOp);
//        AddOperator(&createPAVLOp);
//        AddOperator(&containsOp);
//        AddOperator(&insertOp);
//
//        AddOperator(&importObjectOp);
//        importObjectOp.SetUsesArgsInTypeMapping();
//
//        AddOperator(&importObject2Op);
//        importObject2Op.SetUsesArgsInTypeMapping();
//
//        AddOperator(&text2vstringOp);
//
//        AddOperator(&reverseStreamOp);
//        reverseStreamOp.SetUsesMemory();

        }
    };

} // End namespace

extern "C"
Algebra *
InitializeKafkaAlgebra(NestedList *nlRef,
                       QueryProcessor *qpRef) {
    return new gstguide::KafkaAlgebra;
}

