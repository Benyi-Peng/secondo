/*
----
This file is part of SECONDO.

Copyright (C) 2010, University in Hagen, 
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


1 RegionTools2


This file contains functions handling with regions of the Region2-Algebra and 
the MovingRegion3-Algebra.


*/

/*
1.1 Function ~gmpTypeToTextType()~

Reads from inValue and stores its representation as TextType in resultList.

*/
static void gmpTypeToTextType1
	(const mpq_class& inValue, ListExpr& resultList) {
  stringstream theStream;
  theStream << inValue;
  resultList = nl->TextAtom();
  
  string st = theStream.str();
  nl->AppendText(resultList, st);
}


/*
1.1 Function ~gmpTypeToTextType()~

Reads from inValue and stores its representation as TextType in resultList.

*/
static void gmpTypeToTextType2 (const mpq_class& inValue, ListExpr& resultList) 
{
  stringstream theStream;
  mpf_class helper(inValue,1024);
  
  theStream << setprecision(1024) << helper;
  resultList = nl->TextAtom(theStream.str());
}


/*
1.1 Function ~textTypeToGmpType()~

Reads from inList and stores its representation as mpq\_class in outValue.

*/
static void textTypeToGmpType1
	(const ListExpr& inList, mpq_class& outValue) {

	TextScan theScan = nl->CreateTextScan(inList);
	stringstream theStream;

	char lastChar = '*'; //just a random initialization...

//cout << "Textscan: Länge=" << nl->TextLength(inList) << endl;
//cout << "verbleibend: " ;
	
	for (unsigned int i = 0; i < nl->TextLength(inList); i++)
	{
	  string str = "";
	  nl->GetText(theScan, 1, str);
//cout << "i="<< i << "/" << nl->TextLength(inList) << " ";

	  //Checking for valid character
	  if ((int)str[0] < 47 || (int)str[0] > 57
		|| (i == 0 && (int)str[0] == 48
				&& nl->TextLength(inList) > 1)
		|| (lastChar == '/' && (int)str[0] == 48))
	  {
		stringstream message;
		message << "Precise coordinate not valid: "
		  << nl->ToString(inList)
		  << endl << "Only characters 1, 2, 3, 4, 5, "
			"6, 7, 8, 9, 0 and / allowed." << endl
		  << "0 mustn't be leading "
			"character when "
			"more than one character "
			"in total are given"
		  << endl << ", and 0 is "
			"not allowed directly after /";
		throw invalid_argument(message.str());
	  }

	  theStream.put(str[0]);
	  lastChar = str[0];
	}

//cout << endl;	
	theStream >> outValue;

	outValue.canonicalize();

//cout << outValue << endl;;
	//Checking the value - must be between 0 and 1
	if (cmp(outValue, 1) >= 0 || cmp(outValue,0) < 0)
	{
	  stringstream message;
	  message << "Precise coordinate not valid: "
		<< nl->ToString(inList)
		<< endl
		<< "Resulting value is not between 0 and 1, "
		"where 0 is allowed, but 1 is not.";
	  throw invalid_argument(message.str());
	}
}


/*
1.1 Function ~textTypeToGmpType2()~

Reads from inList and stores its representation as mpq\_class in outValue.

*/
static void textTypeToGmpType2
	(const ListExpr& inList, mpq_class& outValue) {

	TextScan theScan = nl->CreateTextScan(inList);
	stringstream numStream, denStream;
	bool denStart = false;
	denStream.put('1');

	for (unsigned int i = 0; i < nl->TextLength(inList); i++)
	{
	  string str = "";
	  nl->GetText(theScan, 1, str);
	  //Checking for valid character
	  // CHECK FOR  	+/- 3.4e +/- 38 (~7 digits)   ????
	  if ( (int)str[0] < 43  || (int)str[0] > 57  
	    || (int)str[0] == 47 || (int)str[0] == 44
	    || ((i != 0 || nl->TextLength(inList) == 1) 
            && ((int)str[0] == 43 || (int)str[0] == 45)) )
	  {
		stringstream message;
		message << "Precise coordinate not valid: "
		  << nl->ToString(inList)
		  << endl << "Only characters 1, 2, 3, 4, 5, "
			"6, 7, 8, 9, 0 and . allowed." << endl
		  << "+/- must be leading "
			"character when "
			"more than one character "
			"in total are given" << endl;
		throw invalid_argument(message.str());
	  }

	  if (denStart) denStream.put('0');
	  if ((int)str[0] != 46) numStream.put(str[0]);
	  else
	  {
	    denStart = true;
	  }
	}
	outValue = mpq_class(mpz_class(numStream.str()), 
                             mpz_class(denStream.str()));
	outValue.canonicalize();
}


/*
1.1 Function ~D2MPQ()~

Converts value of type double to a value of type mpq\_class 

*/
static mpq_class D2MPQ( const double d )
{
//exact transformation of internal double representation:
  mpq_class res = mpq_class(d);
//return res;

//this version should transform it like WYSIWYG:
  stringstream s, numStream, denStream;
  bool denStart = false;
  denStream.put('1');

  s << d;
  for (unsigned int i = 0; i < s.str().length(); i++)
  {
    if (denStart) denStream.put('0');
    if ((int)s.str()[i] != 46) numStream.put(s.str()[i]);
    else
    {
      denStart = true;
    }
  }
  res = mpq_class(mpz_class(numStream.str(), 10), 
                  mpz_class(denStream.str(), 10));
  res.canonicalize();

  return res;
}


/*
1.1 Function ~SetValueX()~

Stores value z of type mpz\_class in DbArray of type unsigned int, gives back the startposition and 
the number of used unsigned int-values

*/
static void SetValueX(mpz_class z, DbArray<unsigned int>* preciseValuesArray, 
                      int& startpos, int& numofInt) 
{
	if (cmp(z, 0) < 0) return;
  
	if (startpos == -1) 
                startpos = preciseValuesArray->Size();
	int index = startpos;
	if (index >= preciseValuesArray->Size()) 
                preciseValuesArray->resize(index+1);
	numofInt = 0;
	
	mpz_class zdiv(numeric_limits<unsigned int>::max());
	zdiv++;
	mpz_class w(0);
	while ( !(cmp(z, 0) == 0) )
	{
	  w = z%zdiv;
	  unsigned int wert = (unsigned int)w.get_ui();
	  preciseValuesArray->Put(index, wert);
	  index++;
	  numofInt++;
	  z = z/zdiv;
	}
}


/*
1.1 Function ~GetValueX()~

Reads value of type mpz\_class from DbArray of type unsigned int, starting at startposition and 
with number of used unsigned int-values

*/
static mpz_class GetValueX(const int startpos, const int numofInt, 
                           const DbArray<unsigned int>* preciseValuesArray)
{
    mpz_class theValue(0);
    mpz_class zdiv(numeric_limits<unsigned int>::max());
    zdiv++;
    unsigned int wert;
    
    for (int i = startpos + numofInt-1; i >= startpos; i--)
    {
      preciseValuesArray->Get(i, wert);
      theValue = theValue*zdiv + wert;
    }
    return theValue;
}  


