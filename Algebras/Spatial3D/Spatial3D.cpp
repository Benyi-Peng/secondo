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
 
 01590 Fachpraktikum "Erweiterbare Datenbanksysteme" 
 WS 2014 / 2015

<our names here>

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]

[1] Implementation of a Spatial3D algebra

[TOC]

1 Includes and Defines

*/

#include "Spatial3D.h"

#include "Spatial3DSetOps.h"
#include "Spatial3DSTLfileops.h"
#include "Spatial3DOperatorComponents.h"
#include "QueryProcessor.h"
#include "Algebras/Spatial/Point.h"


extern NestedList* nl;
extern QueryProcessor* qp;
extern AlgebraManager* am;




/*
2 Operators

2.1 First Operator....

With some details....

*/


/*
2.1 Type Operator ~<Op1Name>~

bla, bla, bla....

2.1.1 Type mapping function of operator ~group~

----  ((stream x))                -> (rel x)
----

*/

// add typeMapping here

/*
2.1.2 Specification of operator ~<Op1Name>~

*/


// Put operator spec here


/*
2.3.3 Definition of operator ~<Op1Name>~

*/

// Here goe the operator spec




/*
~makepoint3D~

*/
ListExpr makePoint3DTM(ListExpr args){
  if(nl->HasLength(args,2)){
    if(!Point::checkType(nl->First(args))){
      return listutils::typeError("point x {int,real} "
                                  "or {int,real} ^ 3 expected)");
    }
    if(   !CcInt::checkType(nl->Second(args)) 
       && !CcReal::checkType(nl->Second(args))) {
      return listutils::typeError("point x {int,real} "
                                  "or {int,real} ^ 3 expected)");
    }
    return listutils::basicSymbol<Point3d>();
  }

  if(!nl->HasLength(args,3)){
    return listutils::typeError("two or three argumentes expected");
  }
  while(!nl->IsEmpty(args)){
    ListExpr first = nl->First(args);
    args = nl->Rest(args);
    if(!CcInt::checkType(first) && !CcReal::checkType(first)){
      return listutils::typeError("expected {int,real} x "
                                  "{int,real} x {int,real}");
    }
  }
  return listutils::basicSymbol<Point3d>();
}

template<class X, class Y, class Z>
int makePoint3DVMT(Word* args, Word& result, int message, 
                   Word& local,Supplier s){
  result = qp->ResultStorage(s);
  Point3d* res = (Point3d*) result.addr;
  X* x = (X*) args[0].addr;
  Y* y = (Y*) args[1].addr;
  Z* z = (Z*) args[2].addr;
  if(!x->IsDefined() || !y->IsDefined() || !z->IsDefined()){
    res->SetDefined(false);
  } else {
    res->set(x->GetValue(), y->GetValue(),z->GetValue());
  }
  return 0;
}

template<class Z>
int makePoint3DVMT(Word* args, Word& result, int message, 
                   Word& local,Supplier s){
  result = qp->ResultStorage(s);
  Point3d* res = (Point3d*) result.addr;
  Point* p = (Point*) args[0].addr;
  Z* z = (Z*) args[1].addr;
  if(!p->IsDefined() || !z->IsDefined()){
    res->SetDefined(false);
  } else {
    res->set(p->GetX(), p->GetY(),z->GetValue());
  }
  return 0;
}

ValueMapping makePoint3DVM[] = {
   makePoint3DVMT<CcInt,CcInt,CcInt>,
   makePoint3DVMT<CcInt,CcInt,CcReal>,
   makePoint3DVMT<CcInt,CcReal,CcInt>,
   makePoint3DVMT<CcInt,CcReal,CcReal>,
   makePoint3DVMT<CcReal,CcInt,CcInt>,
   makePoint3DVMT<CcReal,CcInt,CcReal>,
   makePoint3DVMT<CcReal,CcReal,CcInt>,
   makePoint3DVMT<CcReal,CcReal,CcReal>,
   makePoint3DVMT<CcInt>,
   makePoint3DVMT<CcReal>
};

int makePoint3DSelect(ListExpr args){
  if(nl->HasLength(args,3)){
    int n1 = CcInt::checkType(nl->First(args))?0:4;
    int n2 = CcInt::checkType(nl->Second(args))?0:2;
    int n3 = CcInt::checkType(nl->Third(args))?0:1;
    return n1 + n2 + n3;
  }
  return CcInt::checkType(nl->Second(args))?8:9;
}

OperatorSpec makePoint3DSpec(
  " {int,real} x {int,real} x {int,real} -> point3d "
  " or point  x {int,real} -> point3d",
  " makePoint3D(_,_,_)",
  "Creates a new point in R^3 from the arguments",
  "query makePoint3D(1, 3.5, 17)"
);

Operator makePoint3DOp(
   "makePoint3D",
   makePoint3DSpec.getStr(),
   10,
   makePoint3DVM,
   makePoint3DSelect,
   makePoint3DTM
);


/*

3 Class ~Spatial3DAlgebra~

A new subclass ~Spatial3DAlgebra~ of class ~Algebra~ is declared. The only
specialization with respect to class ~Algebra~ takes place within the
constructor: all type constructors and operators are registered at the
actual algebra.

After declaring the new class, its only instance ~spatial3DAlgebra~
is defined.

*/



 
class Spatial3DAlgebra : public Algebra
{
 public:
  Spatial3DAlgebra() : Algebra()
  {
    AddTypeConstructor(&Point3dTC);
    Point3dTC.AssociateKind(Kind::DATA());
    AddTypeConstructor(&Surface3dTC);
    Surface3dTC.AssociateKind(Kind::DATA());
    AddTypeConstructor(&Volume3dTC);
    Volume3dTC.AssociateKind(Kind::DATA());

    AddTypeConstructor(&Vector3dTC);
    Vector3dTC.AssociateKind(Kind::DATA());
    AddTypeConstructor(&Plane3dTC);
    Vector3dTC.AssociateKind(Kind::DATA());
    
    //To be merged here after completion...
    AddOperator(spatial3DSTLfileops::getImportSTLptr(),true);
    //To be merged here after completion...
    AddOperator(spatial3DSTLfileops::getExportSTLptr(),true); 

    
    //To be merged here after completion...
    AddOperator(spatial3DTransformations::getRotatePtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DTransformations::getMirrorPtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DTransformations::getTranslatePtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DTransformations::getScaleDirPtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DTransformations::getScalePtr(),true);
    //To be merged here after completion...
    AddOperator(spatial3DOperatorSize::getSizePtr(),true);
    AddOperator(spatial3d_geometric::getTestPtr(),true);
     //To be merged here after completion...
    AddOperator(spatial3DOperatorBBox::getBBoxPtr(),true);
    
    //To be merged here after completion...
    AddOperator(spatial3DCreate::getCreateCubePtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DCreate::getCreateCylinderPtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DCreate::getCreateConePtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DCreate::getCreateSpherePtr(),true); 
    
    //To be merged here after completion...
    AddOperator(spatial3DSetOps::getUnionPtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DSetOps::getIntersectionPtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DSetOps::getMinusPtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DOperatorComponents::getComponentsPtr(),true); 

    //To be merged here after completion...
    AddOperator(spatial3DConvert::getRegion2SurfacePtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DConvert::getRegion2VolumePtr(),true); 
    //To be merged here after completion...
    AddOperator(spatial3DConvert::getMRegion2VolumePtr(),true); 

    AddOperator(&makePoint3DOp);

  }

  ~Spatial3DAlgebra() {};

 private:
  
  GenTC<Point3d> Point3dTC;
  GenTC<Surface3d> Surface3dTC;
  GenTC<Volume3d> Volume3dTC;
  
  GenTC<Vector3d> Vector3dTC;
  GenTC<Plane3d> Plane3dTC;
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

extern "C"
Algebra*
InitializeSpatial3DAlgebra( NestedList* nlRef,
                              QueryProcessor* qpRef,
                              AlgebraManager* amRef )
{
  nl = nlRef;
  qp = qpRef;
  am = amRef;
  return (new Spatial3DAlgebra());
}
