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
#ifndef _Partitionier_h_
#define _Partitionier_h_

#include "Algebras/Collection/CollectionAlgebra.h"
#include "DRelHelpers.h"
#include "DRel.h"

#define DRELDEBUG

extern NestedList* nl;
extern QueryProcessor* qp;

namespace distributed2 {

    ListExpr partitionTM(ListExpr args);

    template<class A>
    int partitionVMT( Word* args, Word& result, int message,
                    Word& local, Supplier s );

    ListExpr areduceTM( ListExpr args );

    template<class R>
    int areduceVMT( Word* args, Word& result, int message,
                Word& local, Supplier s );
}

namespace drel {
/*
1 ~Partitionier~

Class to repartition a DRel.

*/
    template<class R, class T>
    class Partitionier {

    public:
/*
1.1 Constructor

*/
        Partitionier( std::string _attr, std::string _attrType, R* _drel, 
            ListExpr _dType, int _port, std::string _boundaryName ) :
            attr( _attr ), attrType( _attrType ), drel( _drel ), 
            dType( _dType ), count( -1 ), boundary( 0 ), qp( 0 ), matrix( 0 ),
            port( _port ), boundaryName( _boundaryName ) {
        }

/*
1.2 Copy-Constructor

*/
        Partitionier( const Partitionier& src ) :
            attr( src.attr ), attrType( src.attrType ), drel( src.drel ), 
            dType( src.dType ), count( src.count ), 
            boundary( *( src.boundary ) ), qp( 0 ), matrix( *( src.matrix ) ),
            port( src.port ), boundaryName( src.boundaryName ) {
        }

/*
1.2 Assignment operator

*/
        Partitionier& operator=( const Partitionier& src ) {
            if( this == &src ) {
                return *this;
            }
            attr = src.attr;
            attrType = src.attrType;
            drel = src.drel;
            dType = src.dType;
            boundary = src.boundary;
            matrix = src.matrix;
            port = src.port;
            boundaryName = src.boundaryName;
            qp = 0;

            return *this;
        }

/*
1.3 Destructor

*/
        ~Partitionier( ) {
            delete boundary;
            if( qp ) {
                qp->Destroy( tree, true );
                delete qp;
            }

            if( boundary ) {
                delete boundary;
            }
        }

/*
1.4 ~computeBoundary~

Computes a boundary object.

*/
        bool computeBoundary( ) {

            cout << endl;
            cout << "Start: Create boundary object ..." << endl;

            if( count < 0 ) {
                if( !countDRel( ) ) {
                    return false;
                }
            }

            int sampleSize = DRelHelpers::computeSampleSize( count );
            int nthSample = DRelHelpers::everyNthTupleForSample( 
                sampleSize, count );

            std::string query =
            "(createboundary (sort (dsummarize (dmap (convert2darray "
            "(" + nl->ToString( dType ) + " (ptr " + 
            nl->ToString( listutils::getPtrList( drel ) ) + "))) \"\" "
            "(fun (dmapelem1 ARRAYFUNARG1) (project (nth (feed "
            "dmapelem1) " + std::to_string( nthSample ) + " FALSE) (" + attr + 
            ")))))) " + attr + " " + std::to_string( drel->getSize( ) ) + " " +
            std::to_string( sampleSize ) + ")";

            Word result;
            if( !QueryProcessor::ExecuteQuery( query, result ) ) {
                cout << "ERROR: Create boundary object failed!" << endl;
                return false;
            }

            boundary = static_cast<collection::Collection*>( result.addr );

            if( !boundary ) {
                cout << "ERROR: Create boundary object failed!" << endl;
                return false;
            }

            if( !boundary->IsDefined( ) ) {
                cout << "ERROR: Create boundary object failed!" << endl;
                delete boundary;
                boundary = 0;
                return false;
            }

            cout << "Done. Boundary object created!" << endl;

            return true;
        }

/*
1.5 ~countDRel~

Computes the number of tuple in the given drel.

*/
        bool countDRel( ) {

            cout << endl;
            cout << "Start: Compute the size of the drel ..." << endl;

            std::string query =
            "(tie (getValue (dmap (convert2darray (" + nl->ToString( dType ) + 
            " (ptr " + nl->ToString( listutils::getPtrList( drel ) ) + ")))"
            " \"\" (fun (dmapelem1 ARRAYFUNARG1) (count dmapelem1)))) "
            "(fun (first2 ELEMENT) (second3 ELEMENT) (+first2 second3)))";

            Word result;
            if( !QueryProcessor::ExecuteQuery( query, result ) ) {
                cout << "ERROR: Computation of the drel size failed!" << endl;
                return false;
            }

            CcInt* res = ( CcInt* )result.addr;

            if( !res ) {
                cout << "ERROR: Computation of the drel size failed!" << endl;
                return false;
            }

            if( !res->IsDefined( ) ) {
                cout << "ERROR: Computation of the drel size failed!" << endl;
                delete res;
                count = 0;
                return false;
            }

            count = res->GetValue( );
            delete res;

            cout << "Done. DRel size: " + std::to_string( count ) << endl;

            return true;
        }

/*
1.6 ~shareBoundary~

Copies the boundary object to all workers.

*/
        bool shareBoundary( ) {

            cout << endl;
            cout << "Start: Bringh boundary object to the workers ..." << endl;

            if( !boundary ) {
                if( !computeBoundary( ) ) {
                    return false;
                }
            }

            std::string query =
            "(share2 \"" + boundaryName + "\" ((vector " + attrType + ") "
            "(ptr " + nl->ToString( listutils::getPtrList( boundary ) ) + 
            ")) TRUE (convert2darray (" + nl->ToString( dType ) +
            " (ptr " + nl->ToString( listutils::getPtrList( drel ) ) + 
            "))))";

            Word result;
            if( !QueryProcessor::ExecuteQuery( query, result ) ) {
                cout << "ERROR: Bring boundary to the workers failed!" << endl;
                return false;
            }

            FText* res = ( FText* )result.addr;

            if( !res ) {
                cout << "ERROR: Bring boundary to the workers failed!" << endl;
                return false;
            }

            if( !res->IsDefined( ) ) {
                cout << "ERROR: Bring boundary to the workers failed!" << endl;
                delete res;
                return false;
            }

            delete res;

            cout << "Done. Boundary object is now on the workers!" << endl;

            return true;
        }

/*
1.7 ~repartition2DFMatrix~

Repartitions the drel to a DFMatrix.

*/
        bool repartition2DFMatrix( ) {

            cout << endl;
            cout << "Start: Create new partitioning on the workers "
                "as a DFMatrix ..." << endl;

            if( !boundary ) {
                if( !shareBoundary( ) ) {
                    return false;
                }
            }

            ListExpr partitionTMArg = nl->FourElemList(
                nl->TwoElemList(
                    getArrayType( ),
                    nl->SymbolAtom( "dummy" ) ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "dummy" ) ),
                nl->TwoElemList(
                    nl->ThreeElemList(
                        nl->SymbolAtom( "map" ),
                        nl->Second( nl->Second( dType ) ),
                        listutils::basicSymbol<CcInt>( ) ),
                    nl->ThreeElemList(
                        nl->SymbolAtom( "fun" ),
                        nl->TwoElemList(
                            nl->SymbolAtom( "elem_1" ),
                            nl->SymbolAtom( "SUBSUBTYPE1" ) ),
                        nl->ThreeElemList(
                            nl->SymbolAtom( "getboundaryindex" ),
                            nl->SymbolAtom( boundaryName ),
                            nl->ThreeElemList(
                                nl->SymbolAtom( "attr" ),
                                nl->SymbolAtom( "elem_1" ),
                                nl->SymbolAtom( attr ) ) ) ) ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcInt>( ),
                    nl->IntAtom( 0 ) ) );

            ListExpr resultType = distributed2::partitionTM( partitionTMArg );

            ListExpr matrixType = nl->Third( resultType );
            if( !distributed2::DFMatrix::checkType( matrixType ) ) {
                cout << "ERROR: Create new partitioning failed!" << endl;
                 return false;
            }

            // first append value
            FText* fun = new FText( true, 
                nl->TextValue( nl->First( nl->Second( resultType ) ) ) );

            ListExpr query = nl->FiveElemList(
                nl->SymbolAtom( "partition" ),
                createDRelPointerList( ), 
                nl->StringAtom( "" ),
                nl->ThreeElemList(
                    nl->SymbolAtom( "fun" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "elem_1" ),
                        nl->SymbolAtom( "SUBSUBTYPE1" ) ),
                    nl->ThreeElemList(
                        nl->SymbolAtom( "getboundaryindex" ),
                        createBoundaryPointerList( ),
                        nl->ThreeElemList(
                            nl->SymbolAtom( "attr" ),
                            nl->SymbolAtom( "elem_1" ),
                            nl->SymbolAtom( attr ) ) ) ),
                nl->IntAtom( 0 ) );
            
            QueryProcessor* qp = new QueryProcessor( nl, am );
            OpTree tree = createPartitionOpTree( qp, query );

            if( tree == 0 ) {
                cout << "ERROR: Create new partitioning failed!" << endl;
                return false;
            }

            Word result, local, dummy;
            ArgVector argVec = { drel, new CcString( true, "" ), 
                dummy, new CcInt( true , 0 ), fun };

            // call partition value mapping
            distributed2::partitionVMT<T>( 
                argVec, result, 0, local, tree );

            matrix = ( distributed2::DFMatrix* )result.addr;

            if( !matrix ) {
                cout << "ERROR: Create new partitioning failed!" << endl;
                qp->Destroy( tree, true );
                delete qp;
                matrix = 0;
                return false;
            }

            if( !matrix->IsDefined( ) ) {
                cout << "ERROR: Create new partitioning failed!" << endl;
                qp->Destroy( tree, true );
                delete qp;
                matrix = 0;
                return false;
            }

            qp->Destroy( tree, false );
            delete qp;

            cout << "Done. New partitioning is created on the workers!" 
                 << endl;

            return true;
        }

/*
1.8 ~repartition2DFArray~

Repartitions the drel to a DFArray. No function is used. It will 
create a stream without any other functions.

*/
        distributed2::DFArray* repartition2DFArray( ) {

            if( !matrix ) {
                repartition2DFMatrix( );
            }

            ListExpr fun = createFunList(
                nl->TwoElemList( 
                    nl->SymbolAtom( "feed" ),
                    nl->SymbolAtom( "elem_1" ) ) );

            return repartition2DFArray( fun );
        }

/*
1.9 ~repartition2DFArraySortBy~

Repartitions the drel to a DFArray and sorts the relation by the partitioning
attribute.

*/
        distributed2::DFArray* repartition2DFArraySortBy( ) {

            if( !matrix ) {
                repartition2DFMatrix( );
            }

            ListExpr fun = createFunList(
                nl->ThreeElemList(
                    nl->SymbolAtom( "sortby" ),
                    nl->TwoElemList( 
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "elem_1" ) ),
                    nl->OneElemList(
                        nl->ThreeElemList(
                            nl->SymbolAtom( "attr" ),
                            nl->SymbolAtom( "elem_1" ),
                            nl->SymbolAtom( attr ) ) ) ) );

            return repartition2DFArray( fun );
        }

/*
1.10 ~repartition2DFArray~

Repartitions the drel to a DFArray and uses a function while repartitioning.

*/
        distributed2::DFArray* repartition2DFArray( ListExpr funList ) {

            cout << endl;
            cout << "Start: Bring the new partitions to the workers ..." 
                 << endl;

            if( !matrix ) {
                repartition2DFMatrix( );
            }

            ListExpr matrixType = nl->TwoElemList(
                listutils::basicSymbol<distributed2::DFMatrix>( ),
                nl->Second( dType ) );

            // create input for areduce type mapping
            ListExpr areduceTMArg = nl->FourElemList(
                nl->TwoElemList(
                    matrixType,
                    nl->SymbolAtom( "dummy" ) ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                nl->TwoElemList(
                    nl->ThreeElemList(
                        nl->SymbolAtom( "map" ),
                        nl->TwoElemList(
                            nl->SymbolAtom( "fsrel" ),
                            nl->Second( nl->Second( dType ) ) ),
                        nl->TwoElemList(
                            nl->SymbolAtom( "stream" ),
                            nl->Second( nl->Second( dType ) ) ) ),
                    funList ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcInt>( ),
                    nl->IntAtom( port ) ) );

            // call areduce type mapping
            ListExpr resultType = distributed2::areduceTM( areduceTMArg );

            if( !nl->HasLength( resultType, 3 ) ) {
                cout << "ERROR: Transport of partitions failed!" << endl;
                return 0;
            }

            ListExpr darrayType = nl->Third( resultType );
            if( !distributed2::DFArray::checkType( darrayType )
             && !distributed2::DArray::checkType( darrayType ) ) {
                 cout << "ERROR: Transport of partitions failed!" << endl;
                 return 0;
            }

            // first append value
            FText* fun = new FText( true, 
                nl->TextValue( nl->First( nl->Second( resultType ) ) ) );

            // second append value
            CcBool* stream = new CcBool( true,
                nl->BoolValue( nl->Second( nl->Second( resultType ) ) ) );

            ListExpr query = nl->FiveElemList(
                nl->SymbolAtom( "areduce" ),
                nl->TwoElemList(
                    matrixType,
                    nl->TwoElemList(
                        nl->SymbolAtom( "ptr" ),
                        listutils::getPtrList( matrix ) ) ),
                nl->StringAtom( "" ),
                funList,
                nl->IntAtom( port ) );

            QueryProcessor* qp = new QueryProcessor( nl, am );
            OpTree tree = createPartitionOpTree( qp, query );

            Word result, local, dummy;
            ArgVector argVec = { drel, new CcString( true, "" ), 
                dummy, new CcInt( true , port ), fun, stream };

            distributed2::areduceVMT<distributed2::DFArray>( 
                argVec, result, 0, local, tree );

            distributed2::DFArray* resArray = 
                ( distributed2::DFArray* )result.addr;

            qp->Destroy( tree, false );
            delete qp;

            cout << "Done. Repartitioning finished!" 
                 << endl;

            return resArray;
        }

/*
1.11 get functions

This functions will compute the objects if they are not already created.

1.11.1 ~getCount~

*/
        int getCount( ) {

            if( count < 0 ) {
                countDRel( );
            }

            return count;
        }


/*
1.11.2 ~getBoundary~

*/
        collection::Collection* getBoundary( ) {

            if( !boundary ) {
                computeBoundary( );
            }

            return boundary;
        }

/*
1.11.3 ~getDFMatrix~

*/
        distributed2::DFMatrix* getDFMatrix( ) {

            if( !matrix ) {
                repartition2DFMatrix( );
            }

            return matrix;
        }

/*
1.12 set functions

1.12.1 ~setBoundary~

*/

        void setBoundary( collection::Collection* _boundary ) {

            if( boundary == _boundary ) {
                return;
            }

            if( boundary ) {
                boundary->DeleteIfAllowed( );
            }

            boundary = _boundary;
        }

    private:

/*
1.12 ~createFunList~

*/
        ListExpr createFunList( ListExpr list ) {

            ListExpr fun = nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "elem_1" ),
                    nl->SymbolAtom( "AREDUCEARG1" ) ),
                list );

            return fun;
        }

/*
1.13 ~createFunString~

*/
        ListExpr createFunString( ListExpr list ) {

            ListExpr fun = nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "elem_1" ),
                    nl->SymbolAtom( "AREDUCEARG1" ) ),
                list );

            return fun;
        }

/*
1.14 ~createDRelPointerList~

*/
        ListExpr createDRelPointerList( ) {

            return nl->TwoElemList(
                getArrayType( ),
                nl->TwoElemList(
                    nl->SymbolAtom( "ptr" ),
                    listutils::getPtrList( drel ) ) );
        }

/*
1.15 ~getArrayType~

*/
        ListExpr getArrayType( ) {
            ListExpr arrayType;

            if( nl->ToString( nl->First( dType ) ) == DRel::BasicType( ) ) {
                arrayType = nl->TwoElemList(
                    listutils::basicSymbol<distributed2::DArray>( ),
                    nl->Second( dType ) );
            }
            else if( nl->ToString( nl->First( dType ) ) == DFRel::BasicType( ) ) {
                arrayType = nl->TwoElemList(
                    listutils::basicSymbol<distributed2::DFArray>( ),
                    nl->Second( dType ) );
            }

            return arrayType;
        }

/*
1.16 ~createBoundaryPointerList~

*/
        ListExpr createBoundaryPointerList( ) {

            return nl->TwoElemList(
                nl->TwoElemList(
                    nl->SymbolAtom( "vector" ),
                    nl->SymbolAtom( attrType ) ),
                nl->TwoElemList(
                    nl->SymbolAtom( "ptr" ),
                    listutils::getPtrList( boundary ) ) );
        }

/*
1.17 ~createPartitionOpTree~

*/
        OpTree createPartitionOpTree( QueryProcessor* qp, ListExpr query ) {

            bool correct = false;
            bool evaluable = false;
            bool defined = false;
            bool isFunction = false;
            ListExpr resultType;

            qp->Construct(
                query,
                correct,
                evaluable,
                defined,
                isFunction,
                tree,
                resultType );

            return tree;
        }

/*
1.18 Members

*/
        std::string attr;
        std::string attrType;
        R* drel;
        ListExpr dType;
        int count;
        collection::Collection* boundary;
        QueryProcessor* qp;
        distributed2::DFMatrix* matrix;
        int port;
        std::string boundaryName;
        OpTree tree;

    };
    
} // end of namespace drel

#endif // _Partitionier_h_