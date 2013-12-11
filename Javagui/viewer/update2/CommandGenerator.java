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

*/
package viewer.update2;

import java.util.*;

import gui.SecondoObject;
import viewer.*;
import viewer.update.InvalidFormatException;
import viewer.update2.gui.*;
import sj.lang.*;
import sj.lang.ListExpr;
import tools.Reporter;

/**
 * This class generates commands for updating relations in 'Nested-List-Syntax'.
*/
public class CommandGenerator 
{
			
	private CommandExecuter commandExecuter;
	
	// Indices for current relation
	private Vector<String> btreeNames;
	private Vector<String> btreeAttrNames;
	private Vector<String> rtreeNames;
	private Vector<String> rtreeAttrNames;
	
	public CommandGenerator()
	{
		this.commandExecuter = new CommandExecuter();
	}
	
	/*
	 Generates an insert-command for each tuple that shall be inserted. Actualizes the indices as
	 well.
	 */
	public String[] generateInsert(String pRelName, List<String> pAttributeNames, List<String> pAttributeTypes, 
									String[][] insertTuples) throws InvalidFormatException
	{
		
		this.retrieveIndices(pRelName, pAttributeNames);

		String nextValue;
		String nextType;
		int tupleSize = pAttributeTypes.size()-1;
		int tupleCount = insertTuples.length / tupleSize;
		String[] insertCommands = new String[tupleCount];
		
		for (int j = 0; j < tupleCount; j++)
		{
			//StringBuffer insertCommand = new StringBuffer("(query (count ");
			StringBuffer insertCommand = new StringBuffer("(query (consume ");
			
			for (int k = 0; k < btreeNames.size(); k ++)
			{
				insertCommand.append("(insertbtree ");
			}
			
			for (int k = 0; k < rtreeNames.size(); k ++)
			{
				insertCommand.append("(insertrtree ");
			}
			insertCommand.append("(inserttuple " + pRelName + " (");
			
			for (int i = 0; i < tupleSize; i++)
			{
				nextType = pAttributeTypes.get(i).trim();
				ListExpr LE = AttributeFormatter.fromStringToListExpr(nextType, insertTuples[j*tupleSize+i][1]);
				if(LE==null)
				{
					throw new InvalidFormatException("Invalid Format for "+nextType,j*tupleSize+i+1,i+1);
				}
				nextValue = LE.writeListExprToString();				
				insertCommand.append("( "+nextType+" "+nextValue + ") ");		
			}
			insertCommand.append("))");
			
			for (int k = 0; k < rtreeNames.size(); k ++)
			{
				insertCommand.append(rtreeNames.get(k)+ " " + rtreeAttrNames.get(k) + ")");
			}
			
			for (int k = 0; k < btreeNames.size(); k ++)
			{
				insertCommand.append(btreeNames.get(k)+ " " + btreeAttrNames.get(k) + ")");
			}
			insertCommand.append("))");
			Reporter.debug("CommandGenerator.generateInsert: " + insertCommand.toString());
			insertCommands[j] = insertCommand.toString();
		}
		return insertCommands;	
	}
	
	/*
	 * Generates a delete-command for each tuple that shall be deleted. Actualizes the indices as
	 * well.
	 */
	public List<String> generateDelete(String pRelName, List<String> pAttributeNames, List<String> pDeletedTupleIds)
	{
        this.retrieveIndices(pRelName, pAttributeNames);

		List<String> deleteCommands = new ArrayList<String>();
		StringBuffer deleteCommand;
		
		for (String id : pDeletedTupleIds)
		{
			deleteCommand = new StringBuffer("(query (count ");
			
			for (int k = 0; k < btreeNames.size(); k ++){
				deleteCommand.append("(deletebtree ");
			}
			
			for (int k = 0; k < rtreeNames.size(); k ++){
				deleteCommand.append("(deletertree ");
			}
			
			deleteCommand.append("(deletebyid " + pRelName + " (tid " + id + " )) " );

			for (int k = 0; k < rtreeNames.size(); k ++)
			{
				deleteCommand.append(rtreeNames.get(k)+ " " + rtreeAttrNames.get(k) + ")");
			}
			
			for (int k = 0; k < btreeNames.size(); k ++)
			{
				deleteCommand.append(btreeNames.get(k)+ " " + btreeAttrNames.get(k) + ")");
			}			
			deleteCommand.append("))");
			
			deleteCommands.add(deleteCommand.toString());
		}
		return deleteCommands;	
	}

	
	/*
	* Generates an update-command for each changed tuple
	* including actualization of indices.
	*/
	public List<String> generateUpdate(String pRelName, List<String> pAttributeNames, 
										Map<Integer, HashMap<String, Change>> pChanges) 
										throws InvalidFormatException
	{
		this.retrieveIndices(pRelName, pAttributeNames);
		
		List<String> updateCommands = new ArrayList<String>();
		
		int index;
		String type;
		String value;
		String leString;
		Map<String, Change> tupleChanges;
		Change attrChange;
		
		for (Integer tid : pChanges.keySet())
		{
			tupleChanges = pChanges.get(tid);
			
			Reporter.debug("viewer.update2.CommandGenerator.generateUpdate: tuple ID is " + tid);
			
			StringBuffer updateCommand = new StringBuffer("(query (count  " );
			
			for (int k = 0; k < this.btreeNames.size(); k ++)
			{
				updateCommand.append("(updatebtree ");
			}
			
			for (int k = 0; k < this.rtreeNames.size(); k ++)
			{
				updateCommand.append("(updatertree ");
			}
			
			updateCommand.append("(updatebyid " );
			updateCommand.append(pRelName + " (tid " + tid + " ) ");
			updateCommand.append( "(");
			
			for (String name : tupleChanges.keySet())
			{
				attrChange = tupleChanges.get(name);
				index = attrChange.getAttributeIndex();
				type = attrChange.getAttributeType();
				value = attrChange.getNewValue();
				
				updateCommand.append("(" + name);
				updateCommand.append("( fun ( tuple" + (index+1) + " TUPLE )");
				
				ListExpr le = AttributeFormatter.fromStringToListExpr(type, value);
				
				if(le==null)
				{
					throw new InvalidFormatException("Invalid Format for " + type, tid, index+1);
				}
					
				leString = le.writeListExprToString();				
				updateCommand.append("( " + type + " " + leString + ") ");		
				updateCommand.append("))");
			}
			
			updateCommand.append("))");
			
			for (int k = 0; k < rtreeNames.size(); k ++){
				updateCommand.append(rtreeNames.get(k)+ " " + rtreeAttrNames.get(k) + ")");
			}
			
			for (int k = 0; k < btreeNames.size(); k ++){
				updateCommand.append(btreeNames.get(k)+ " " + btreeAttrNames.get(k) + ")");
			}
			
			updateCommand.append("))");
			
			updateCommands.add(updateCommand.toString());
		}
		return updateCommands;	
	}
	
	

	/*
	 Sends a 'list objects'-command to SECONDO and scans the result for all indices for the given relation.
	 To do this it uses the convention that indices have to begin with the relationname with the first
	 letter in lowercase, following an underscore and then the name of the attribute over which
	 the index is built
	 */	
	private void retrieveIndices(String pRelName, List<String> pAttrNames)
	{
		commandExecuter.executeCommand("(list objects)", SecondoInterface.EXEC_COMMAND_LISTEXPR_SYNTAX);
		ListExpr inquiry = commandExecuter.getResultList();
		ListExpr objectList = inquiry.second().second();
		objectList.first();
		ListExpr rest = objectList.rest();
		ListExpr nextObject;
		String name;
		String attrName;
		ListExpr type;
		this.btreeNames = new Vector<String>();
		this.btreeAttrNames = new Vector<String>();
		this.rtreeNames = new Vector<String>();
		this.rtreeAttrNames = new Vector<String>();
		while (! rest.isEmpty())
		{
			nextObject = rest.first();
			type = nextObject.fourth();
			if (!(type.first().isAtom())){
				if ((type.first().first().isAtom())){
					if (type.first().first().symbolValue().equals("btree")){
						name = nextObject.second().symbolValue();
						if (name.indexOf('_') != -1){
							if(name.substring(0,name.indexOf('_')).equalsIgnoreCase(pRelName)){
								if(name.substring(1,name.indexOf('_')).equals(pRelName.substring(1))){
									attrName = name.substring(name.indexOf('_') + 1);
									for (int i = 0; i < pAttrNames.size(); i++){
										if (attrName.trim().equals(pAttrNames.get(i))){
											this.btreeNames.add(name);
											this.btreeAttrNames.add(pAttrNames.get(i));
										}
									}
								}
							}
						}
					}
					else {
						if (type.first().first().symbolValue().equals("rtree")){
							name = nextObject.second().symbolValue();
							if (name.indexOf('_') != -1){
								if(name.substring(0,name.indexOf('_')).equalsIgnoreCase(pRelName)){
									if(name.substring(1,name.indexOf('_')).equals(pRelName.substring(1))){
										attrName = name.substring(name.indexOf('_') + 1);
										for (int i = 0; i < pAttrNames.size(); i++){
											if (attrName.trim().equals(pAttrNames.get(i))){
												this.rtreeNames.add(name);
												this.rtreeAttrNames.add(pAttrNames.get(i));
											}
										}
									}
								}
							}
						}
					}
				}
			}
			rest = rest.rest();
		}
	}
	
	
	
}
