/*

1.1 ~CommandBuilder~

The \textit{CommandBuilder} provides functions that can be used for creating,
inserting tuples into and updating tuples of relations.

----
This file is part of SECONDO.

Copyright (C) 2017,
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
#ifndef ALGEBRAS_DBSERVICE_COMMANDBUILDER_HPP_
#define ALGEBRAS_DBSERVICE_COMMANDBUILDER_HPP_

#include <sstream>
#include <string>
#include <array>
#include <utility>
#include <tuple>
#include <vector>

#include "RelationInfo.hpp" //TODO fix test makefile

namespace DBService
{

/*

1.1.1 Type Definitions

1.1.1.1 \textit{AttributeType}

This enum covers all data types that can be used in commands generated by the
\textit{CommandBuilder}.

*/

enum AttributeType
{
    STRING = 1,
    INT = 2,
    BOOL = 3,
    TEXT = 4
};

/*
1.1.1.1 \textit{AttributeInfo}

This struct combines an \textit{AttributeType} with the name of the attribute.

*/

struct AttributeInfo
{
    AttributeType type;
    std::string name;
};

/*
1.1.1.1 \textit{AttributeInfoWithValue}

This struct combines an \textit{AttributeType} with the name of the attribute
and the corresponding value.

*/

struct AttributeInfoWithValue
{
    AttributeInfo attributeInfo;
    std::string value;
};

/*
1.1.1.1 \textit{RelationDefinition}

A relation consists of a number of attributes, therefore a vector of
\textit{AttributeInfo} is defined as \textit{RelationDefinition}.

*/

typedef std::vector<AttributeInfo> RelationDefinition;

/*
1.1.1.1 \textit{FilterConditions}

The \textit{UpdateRelationAlgebra} performs update and delete commands on a
tuple stream. To delete certain tuples, we therefore need to provide the
\textit{CommandBuilder} with filter conditions that include the type, the name
and the value of an attribute. We might want to provide more than one filter
condition, therefore we define a vector of \textit{AttributeInfoWithValue} as
\textit{FilterConditions}.

*/

typedef std::vector<AttributeInfoWithValue> FilterConditions;

/*
1.1.1 Class Definition

*/

class CommandBuilder {
public:
/*
1.1.1.1 \textit{getTypeName}

This function returns a string containing the respective type name for a
given \textit{AttributeType}.

*/
    static std::string getTypeName(AttributeType type);

/*
1.1.1.1 \textit{buildCreateCommand}

This function returns a string containing the SECONDO command for
creating a relation with the given name (\textit{relationName}) and attributes
(\textit{rel}). When executing the command, the created relation will contain
 one tuple, specified by the given \textit{values}.

*/
    static std::string buildCreateCommand(
            const std::string& relationName,
            const RelationDefinition& rel,
            const std::vector<std::vector<std::string> >& values);

/*
1.1.1.1 \textit{buildInsertCommand}

This function returns a string containing the SECONDO command for
inserting one tuple (\textit{values}) into a relation with the given name
(\textit{relationName}) and attributes (\textit{rel}).

*/
    static std::string buildInsertCommand(
            const std::string& relationName,
            const RelationDefinition& rel,
            const std::vector<std::string>& values);

/*
1.1.1.1 \textit{buildUpdateCommand}

This function returns a string containing the SECONDO command for
updating tuples of a relation with the given name (\textit{relationName}).
The tuples that are going to be updated to a new value (\textit{valueToUpdate})
are determined by the given filter conditions (\textit{filterConditions}).

*/
    static std::string buildUpdateCommand(
            const std::string& relationName,
            const FilterConditions& filterConditions,
            const AttributeInfoWithValue& valueToUpdate);
/*
1.1.1.1 \textit{buildDeleteCommand}

This function returns a string containing the SECONDO command for
deleting tuples of a relation with the given name (\textit{relationName}).
The tuples that are going to be deleted are determined by the
given filter conditions (\textit{filterConditions}).

*/
    static std::string buildDeleteCommand(
            const std::string& relationName,
            const std::vector<AttributeInfoWithValue>& filterConditions);
/*
1.1.1.1 \textit{addAttributeValue}

This internally used function adds a given attribute value (\textit{value})
to a stringstream (\textit{stream}) by considering the type of the attribute
(\textit{info}) and thus adding quotes where necessary.

*/
protected:
    static void addAttributeValue(
            std::stringstream& stream,
            const AttributeInfo& info,
            const std::string& value);
};

} /* namespace DBService */

#endif /* ALGEBRAS_DBSERVICE_COMMANDBUILDER_HPP_ */
