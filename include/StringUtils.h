
/*
---- 
This file is part of SECONDO.

Copyright (C) 2011, University in Hagen, Faculty of Mathematics and Computer Science, 
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

1 This file provides some functions handling with strings.

The implentations can be found at StringUtils.cpp in directory Tools/Utilities.

*/


#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>


namespace stringutils {

/*
1 StringTokenizer

This tokenizer splits a string at positions of delimiters. 
Empty tokens are returned but the delimiters are omitted.
 

*/

class StringTokenizer{
   public:
/*
1.1 Constructor

Constructs a tokenizer for the given string and the given delimiters.

*/

       StringTokenizer(const std::string& s, const std::string& _delims);

/*
1.2 Checks whether more tokens are available.

*/

       bool hasNextToken() const;

/*
1.3 getRest

Returns the non processed part of the string.

*/
      std::string getRest() const;


/*
~nextToken~

Returns the next token.

*/
       std::string nextToken();
   private:
       std::string str;
       std::string delims;
       size_t pos;
};

/*
2 ~trim~

removes whitespaces at the begin and at the end of the string.

*/

void trim(std::string& str);

/*
The following function is used to replace all occurences of a pattern within a
string by an other pattern.

*/

std::string replaceAll(const std::string& textStr,
                  const std::string& patternOldStr,
                  const std::string& patternNewStr);

} // end of namespace stringutils

#endif


