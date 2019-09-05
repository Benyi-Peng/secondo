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

#include "StreamStopping.h"
#include "StandardTypes.h"

#include "Attribute.h" // implementation of attribute types
#include "NestedList.h" // required at many places
#include "Operator.h" // for operator creation
#include "ListUtils.h" // useful functions for nested lists
#include "Algebras/Stream/Stream.h" // wrapper for secondo streams
#include "Algebras/Relation-C++/RelationAlgebra.h" // use of tuples
#include "SignalingSockets.h"
#include <stack>

using namespace std;

namespace kafka {

    ListExpr finishStreamTM(ListExpr args) {
        if (!nl->HasLength(args, 2)) {
            return listutils::typeError(" wrong number of args ");
        }

        if (!Stream<Tuple>::checkType(nl->First(args))
            || !CcInt::checkType(nl->Second(args))) {
            return listutils::typeError(" stream(Tuple) x Port "
                                        "expected ");
        }
        return nl->First(args);
    }

    class FinishStreamLI {
    public :
// s is the stream argument , st the int argument
        FinishStreamLI(Word s, CcInt *st) : stream(s) {
            def = st->IsDefined();
            if (def) {
                int port = st->GetValue();
                signallingSocket.open(port); // TODO: return code
            }
            stream.open();
        }

        ~ FinishStreamLI() {
            stream.close();
            if (def) {
                signallingSocket.close(); // TODO: return code
            }
        }

        Tuple *getNext() {
            if (signallingSocket.isSignalReceived()) {
                return 0;
            }
            Tuple *k = stream.request();
            if (k) {
                return k;
            }
            return 0;
        }

    private :
        Stream<Tuple> stream;
        bool def;

        SignallingSocket signallingSocket;
    };


    int finishStreamVM(Word *args, Word &result, int message,
                       Word &local, Supplier s) {
        FinishStreamLI *li = (FinishStreamLI *) local.addr;
        switch (message) {
            case OPEN :
                if (li) {
                    delete li;
                }
                local.addr = new FinishStreamLI(args[0],
                                                (CcInt *) args[1].addr);
                return 0;
            case REQUEST :
                result.addr = li ? li->getNext() : 0;
                return result.addr ? YIELD : CANCEL;
            case CLOSE :
                if (li) {
                    delete li;
                    local.addr = 0;
                }
                return 0;
        }
        return 0;
    }

    OperatorSpec finishStreamSpec(
            " stream ( Tuple ) x Port -> stream ( Tuple ) ",
            " _ finishStream[_]",
            " All tuples in the stream are written to the output, "
            "but stops by receiving stop signal from signalFinish ",
            " query plz feed finishStream(8080) count "
    );

    Operator finishStreamOp(
            "finishStream",
            finishStreamSpec.getStr(),
            finishStreamVM,
            Operator::SimpleSelect,
            finishStreamTM
    );


    ListExpr signalFinishTM(ListExpr args) {
        if (!nl->HasLength(args, 2)) {
            return listutils::typeError(" wrong number of args ");
        }

        if (!CcString::checkType(nl->First(args))
            || !CcInt::checkType(nl->Second(args))) {
            return listutils::typeError(" ip x Port "
                                        "expected ");
        }
        // TODO: Some empty result
        return listutils::basicSymbol<CcReal>();
    }

    int signalFinishVM(Word *args, Word &result, int message,
                       Word &local, Supplier s) {
        // TODO: Implement
        return 0;
    }

    OperatorSpec signalFinishSpec(
            " host x Port -> empty? ",
            " signalFinish(host, port)",
            " Sends finish signal to finishStream operator ",
            " query signalFinish(\"127.0.0.1\", 8080)"
    );

    Operator signalFinishOp(
            "signalFinish",
            signalFinishSpec.getStr(),
            signalFinishVM,
            Operator::SimpleSelect,
            signalFinishTM
    );

}
