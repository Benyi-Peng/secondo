#include <string>
#include <iostream>
#include "SecondoSMI.h"

#define CTABLE_PERSISTENT
#include "CTable.h"


/*   Important Note: Currently the CTable is implemented as a temporary datastructure 
 *   which uses main memory and Berkeley-DB records as disk memory. Functions for
 *   storing (and restoring) a CTable completley on disk are not implemented yet.
 */

SmiRecordFile* rf = 0;

using namespace std;

void pause()
{
  char buf[80];
  cout << "<<< Press return to continue >>>" << endl;
  cin.getline( buf, sizeof(buf) );
}

/*
bool
InstantiateByRecordIdTest() {

 int recId;
 char inChar;

 cout << "CTable von Disk laden (y,n)?";
 cin >> inChar;

 if ( inChar == 'y' || inChar == 'Y' ) {
    
    cout << "Record Id?";	 
    cin >> recId;

    CTable<int> cti(recId,true);
    CTable<int>::Iterator it;
    for ( it = cti.Begin(); it != cti.End(); ++it )
    {
       cout << "it " << *it << ", i=" << it.GetIndex() << endl;
    }
    
 }
	
 return true;
}
*/


bool
PArrayTest() {
	
  cout << "Tests for the PArray template class!" << endl << endl;
  PagedArray<int> pa(rf);

  int max = 100;
  for (int j = 0; j < max; j++) {

  int val = max-j;
  pa.Put(j, val);	  
  }	 

  for (int k = 0; k < max; k++) {
  int val;
  pa.Get(k, val);
  cout << "pa[" << k << "] = " << val << endl; 
  }
  
  return true;
}

bool
PCTableTest() {
	
  cout << "Tests for the PCTable class!" << endl;
	
  bool test = true;
  cout << "true  : " << test << endl;
  test = false;
  cout << "false : " << test << endl;
  
  CTable<bool> bt( 3, rf );
  bool b1;
  
  bt.Get(1, b1);
  b1 = false;
  bt.Put(1, b1);
  
  bt.Get(2, b1); 
  b1 = true;
  bt.Put(2, b1);
  
  cout << "bt[1] false : " << bt[1] << endl;
  cout << "bt[2] true  : " << bt[2] << endl;
  
  CTable<int> ct( 5, rf );
  cout << "size = 5: " << ct.Size() << endl;
 
  int val = 1;
  ct.Put(2, val);
  int intRef = ct[2];
  cout << "intRef = 1: " << intRef << endl;
  intRef = 2;
  val = 40;
  ct.Put(4, val);
  cout << "ct[2] = 1: " << ct[2] << endl;
  cout << "size = 5: " << ct.Size() << endl;
  cout << "ct[4] = 40: " << ct[4] << endl;
  
  cout << "1: " << ct.IsValid( 1 ) << endl;
  cout << "2: " << ct.IsValid( 2 ) << endl;
  cout << "3: " << ct.IsValid( 3 ) << endl;
  cout << "4: " << ct.IsValid( 4 ) << endl;
  cout << "5: " << ct.IsValid( 5 ) << endl;
  ct.Add( 10 );
  ct.Add( 11 );
  ct.Add( 12 );
  ct.Add( 13 );
  ct.Add( 14 );

  // show values
  cout << "values : (10,1,11,40,12,13,14,) --> (";
  for (int k = 1; k <= 7; k++) {
    
     cout << ct[k] << ","; 
  }
  cout << ")" << endl;
  
  // show sizes
  cout << "size 7: " << ct.Size() << endl;
  cout << "NoEntries(): " << ct.NoEntries() << endl;
  
  
  CTable<int>::Iterator it, it2;
  it2 = ct.Begin();
  it = it2++;
  cout << "it  " << *it  << ", i= " << it.GetIndex()  << endl;
  
  ct.Get(it.GetIndex(), val);
  val = 5;
  ct.Put(it.GetIndex(), val);
  
  cout << "it2 " << *it2 << ", i2=" << it2.GetIndex() << endl;
  ct.Remove( 5 );
  for ( it = ct.Begin(); it != ct.End(); ++it )
  {
    cout << "it " << *it << ", i=" << it.GetIndex() << endl;
  }
  cout << "Test for end of scan!" << endl;
  cout << "eos? " << it.EndOfScan() << endl;
  ++it;
  cout << "eos? " << it.EndOfScan() << endl;

  //cout << "Id of CTable: " << ct.Id() << endl;

  char inChar;
  cout << "Delete CTable from Disk when terminating (y/n)?";
  cin >> inChar;

  if ( inChar == 'y' || inChar == 'Y' ) {
  // ct.MarkDelete();
  }

  return true;
}


int
main() {

  SmiError rc;
  bool ok;

  rc = SmiEnvironment::StartUp( SmiEnvironment::MultiUser,
                                "SecondoConfig.ini", cerr );
  cout << "StartUp rc=" << rc << endl;
  if ( rc == 1 )
  {
    string dbname;
    cout << "*** Start list of databases ***" << endl;
    while (SmiEnvironment::ListDatabases( dbname ))
    {
      cout << dbname << endl;
    }
    cout << "*** End list of databases ***" << endl;
 
    ok = SmiEnvironment::OpenDatabase( "PARRAY" );   
    if ( ok )
    {
      cout << "OpenDatabase PARRAY ok." << endl;
    }
    else
    {
      cout << "OpenDatabase PARRAY failed, try to create." << endl;
      ok = SmiEnvironment::CreateDatabase( "PARRAY" );
      if ( ok )
        cout << "CreateDatabase PARRAY ok." << endl;
      else
        cout << "CreateDatabase PARRAY failed." << endl;
      
      if ( SmiEnvironment::CloseDatabase() )
        cout << "CloseDatabase PARRAY ok." << endl;
      else
        cout << "CloseDatabase PARRAY failed." << endl;
      
      if ( ok = SmiEnvironment::OpenDatabase( "PARRAY" ) )
        cout << "OpenDatabase PARRAY ok." << endl;
      else
        cout << "OpenDatabase PARRAY failed." << endl;
    }
    pause();
    if ( ok )
    {
      cout << "Begin Transaction: " << SmiEnvironment::BeginTransaction() << endl;
     
	rf = new SmiRecordFile(false, 0);
	ok = rf->Open("parrayfile");
	cout << "parrayfile opened: " << ok << endl;

 
      //InstantiateByRecordIdTest();
      pause();
      PArrayTest();
      pause();
      PCTableTest();
      
      cout << "Commit: " << SmiEnvironment::CommitTransaction() << endl;
      cout << "*** Closing Database ***" << endl;
      if ( SmiEnvironment::CloseDatabase() )
        cout << "CloseDatabase ok." << endl;
      else
        cout << "CloseDatabase failed." << endl;
      pause();
    }
  }
  rc = SmiEnvironment::ShutDown();
  cout << "ShutDown rc=" << rc << endl;
  
 
  return 0;
 

}



