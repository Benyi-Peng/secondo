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


//[$][\$]

*/

#include "ARRAYORTASKSFUNARG.h"

using namespace std;
using namespace distributed2;

namespace distributed5
{
/*

1 TypeMapOperators ARRAYORTASKSFUNARG1, ARRAYORTASKSFUNARG2 up to ARRAYORTASKSFUNARG8

*/
template <int pos, bool convertToFsrel>
ListExpr ARRAYORTASKSFUNARG_TM(ListExpr args)
{

    if (!nl->HasMinLength(args, pos))
    {
        return listutils::typeError("too few arguments");
    }
    for (int i = 1; i < pos; i++)
    {
        args = nl->Rest(args);
    }
    ListExpr arg = nl->First(args);
    // i.e. arg = (darray int) or (stream (task (darray int)))

    ListExpr innerType;

    // unpack stream
    if (Stream<Task>::checkType(arg))
    {
        arg = Task::innerType(nl->Second(arg));
    }

    // i.e. arg = (darray int)
    if (DArray::checkType(arg))
    {
        // i.e. return int;
        innerType = nl->Second(arg);
    }
    // i.e. arg = (dfarray (rel ...))
    else if (DFArray::checkType(arg))
    {
        // i.e. return int;
        innerType = nl->TwoElemList(listutils::basicSymbol<frel>(),
                                    nl->Second(nl->Second(arg)));
    }
    else
    {
        return listutils::typeError("Invalid type found");
    }

    if (convertToFsrel)
    {
        if (!Relation::checkType(innerType) && !frel::checkType(innerType))
        {
            return listutils::typeError("subtype type is not a relation");
        }
        return nl->TwoElemList(listutils::basicSymbol<fsrel>(),
                               nl->Second(innerType));
    }
    else
    {
        return innerType;
    }
}

OperatorSpec ARRAYORTASKSFUNARG1Spec(
    "d(f)array(X) x ... -> X, stream(task(X)) x ... -> X",
    "ARRAYORTASKSFUNARG1(_)",
    "Type mapping operator.",
    "query df1 dmap_S [\"df3\" . count]");

Operator ARRAYORTASKSFUNARG1Op(
    "ARRAYORTASKSFUNARG1",
    ARRAYORTASKSFUNARG1Spec.getStr(),
    0,
    Operator::SimpleSelect,
    ARRAYORTASKSFUNARG_TM<1, false>);

OperatorSpec ARRAYORTASKSFUNARG2Spec(
    "d(f)array(X) x ... -> X, stream(task(X)) x ... -> X",
    "ARRAYORTASKSFUNARG2(_)",
    "Type mapping operator.",
    "query df1 df2 dmapS2 [\"df3\" . count]");

Operator ARRAYORTASKSFUNARG2Op(
    "ARRAYORTASKSFUNARG2",
    ARRAYORTASKSFUNARG2Spec.getStr(),
    0,
    Operator::SimpleSelect,
    ARRAYORTASKSFUNARG_TM<2, false>);

OperatorSpec ARRAYORTASKSFUNARG3Spec(
    "d(f)array(X) x ... -> X, stream(task(X)) x ... -> X",
    "ARRAYORTASKSFUNARG3(_)",
    "Type mapping operator.",
    "query df1 df2 df3 dmapS3 [\"df3\" . count]");

Operator ARRAYORTASKSFUNARG3Op(
    "ARRAYORTASKSFUNARG3",
    ARRAYORTASKSFUNARG3Spec.getStr(),
    0,
    Operator::SimpleSelect,
    ARRAYORTASKSFUNARG_TM<3, false>);

OperatorSpec ARRAYORTASKSFUNARG4Spec(
    "d(f)array(X) x ... -> X, stream(task(X)) x ... -> X",
    "ARRAYORTASKSFUNARG4(_)",
    "Type mapping operator.",
    "query df1 df2 df3 df4 dmapS4 [\"df4\" . count]");

Operator ARRAYORTASKSFUNARG4Op(
    "ARRAYORTASKSFUNARG4",
    ARRAYORTASKSFUNARG4Spec.getStr(),
    0,
    Operator::SimpleSelect,
    ARRAYORTASKSFUNARG_TM<4, false>);

OperatorSpec ARRAYORTASKSFUNARG5Spec(
    "d(f)array(X) x ... -> X, stream(task(X)) x ... -> X",
    "ARRAYORTASKSFUNARG5(_)",
    "Type mapping operator.",
    "query df1 df2 df3 df4 df5 dmapS5 [\"df5\" . count]");

Operator ARRAYORTASKSFUNARG5Op(
    "ARRAYORTASKSFUNARG5",
    ARRAYORTASKSFUNARG5Spec.getStr(),
    0,
    Operator::SimpleSelect,
    ARRAYORTASKSFUNARG_TM<5, false>);

OperatorSpec ARRAYORTASKSFUNFSARG2Spec(
    "d(f)array(X) x ... -> X, stream(task(X)) x ... -> X",
    "ARRAYORTASKSFUNFSARG2(_)",
    "Type mapping operator.",
    "query df1 df2 dproduct_S [\"df3\" . count]");

Operator ARRAYORTASKSFUNFSARG2Op(
    "ARRAYORTASKSFUNFSARG2",
    ARRAYORTASKSFUNFSARG2Spec.getStr(),
    0,
    Operator::SimpleSelect,
    ARRAYORTASKSFUNARG_TM<2, true>);

} // namespace distributed5
