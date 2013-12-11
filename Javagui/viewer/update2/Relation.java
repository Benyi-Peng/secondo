//This file is part of SECONDO.

//Copyright (C) 2004, University in Hagen, Department of Computer Science, 
//Database Systems for New Applications.

//SECONDO is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version.

//SECONDO is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with SECONDO; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

package viewer.update2;

import gui.SecondoObject;
import gui.idmanager.*;

import java.util.ArrayList;
import java.util.List;

import sj.lang.ListExpr;

import tools.Reporter;


/**
 *
 */
public class Relation
{
	
	private RelationTypeInfo relTypeInfo;
	private List<SecondoObject> SecondoObjects;
	private boolean initialized;
	private String myID;
	private String Name;
	private List<String> TupleIDs;
	private ListExpr TupleType;
	private SecondoObject WholeRelation;
	
	
	
	
	
	public Relation()
	{
		SecondoObjects = new ArrayList<SecondoObject>();
		TupleIDs = new ArrayList<String>();
		relTypeInfo = new RelationTypeInfo(); 
		TupleType = ListExpr.theEmptyList();
		WholeRelation = null;
		initialized = false;
	}
	
	
	/** 
	 * Append new tuple to relation 
	 */
	public void addTuple(ListExpr pTuple)
	{
		if(initialized)
		{
			List<SecondoObject> tupleSOs = this.readTupleValueFromLE(pTuple);
			
			if (tupleSOs.size() == relTypeInfo.getSize())
			{
				for(int i=0; i<relTypeInfo.getSize(); i++)
				{
					SecondoObject SO = tupleSOs.get(i);
					SecondoObjects.add(SO);
					Reporter.debug("Relation.addTuple: SO is " + SO.toString());
					
					int tidindex = this.relTypeInfo.getTidIndex();
					if (tidindex >= 0 && tidindex == i)
					{
						this.TupleIDs.add(SO.toListExpr().second().toString().trim());
					}
				}
			}
		}
	}
	
	
	
	public List<String> getAttributeNames()
	{
		return this.relTypeInfo.getAttributeNames();
	}
	
	public List<String> getAttributeTypes()
	{
		return this.relTypeInfo.getAttributeTypes();
	}

	
	/**
	 * Returns relation name.
	 */
	public String getName(){
		return this.Name;
	}
	
	
	/*
	public int find(String S,boolean CaseSensitiv,int OffSet)
	{
		boolean found =false;
		int pos = -1;
		String US = S.toUpperCase();
		for(int i=OffSet;i<SecondoObjects.size()&&!found;i++)
		{
			String tmpname = get(i).getName();
			if(CaseSensitiv){
				if (tmpname.indexOf(S)>=0)
				{
					found=true;
					pos=i;
				}
			} else{
				if (tmpname.toUpperCase().indexOf(US)>=0)
				{
					found=true;
					pos=i;
				}
				
			}
		}
		return pos;
	}
	 */
	
		
	public boolean readFromSecondoObject(SecondoObject SO)
	{
		myID = SO.getID().toString();
		Name = SO.getName();
		initialized = false;
		ListExpr LE = SO.toListExpr();
		if(LE.listLength()!=2)
		{
			Reporter.writeError("update2.Relation.readFromSecondoObject : wrong list length");
			return false;
		}
		if(!relTypeInfo.readFromRelTypeLE(LE.first()))
		{
			Reporter.writeError("update2.Relation.readFromSecondoObject : wrong type list");
			return false;
		}
		
		if(!readRelationValueFromLE(LE.second()))
		{
			Reporter.writeError("update2.Relation.readFromSecondoObject : wrong value list");
			return false;
		}
		
		WholeRelation = SO;
		TupleType = SO.toListExpr().first().second();
		initialized = true;
		return true;
	}
		
	
	/** read the Value of this Relation */
	private boolean readRelationValueFromLE(ListExpr ValueList)
	{
		ListExpr NextTuple;
		ListExpr Rest = ValueList;
		SecondoObjects.clear();
		TupleIDs.clear();
		WholeRelation = null;
		TupleType = ListExpr.theEmptyList();
		boolean ok = true;
		int T_no = 0;
		while(Rest.listLength()>0 && ok)
		{
			NextTuple = Rest.first();
			T_no++;
			Rest = Rest.rest();
			if(NextTuple.listLength()!=this.relTypeInfo.getSize())  // wrong tuplelength
			{
				ok = false;
			}
			else
			{
				SecondoObject SO;
				int No = 0;
				//TupleIDs.add(IDManager.getNextID().toString());
				while(NextTuple.listLength()>0)
				{
					SO = new SecondoObject(IDManager.getNextID());
					ListExpr Type = ListExpr.symbolAtom(this.relTypeInfo.get(No).Type);
					SO.fromList(ListExpr.twoElemList(Type, NextTuple.first()));
					String aName = computeObjectName(this.relTypeInfo.get(No).Name, 
													 this.relTypeInfo.get(No).Type, 
													 NextTuple.first());
					SO.setName(Name+"::"+aName+"::"+T_no);
					SecondoObjects.add(SO);
					if(this.relTypeInfo.get(No).Name.equals("TID"))
					{
						TupleIDs.add(NextTuple.first().toString().trim());
					}
					
					NextTuple = NextTuple.rest();
					No++;
				}
			}
		}
		if(!ok)
		{
			SecondoObjects.clear();
			TupleIDs.clear();
			TupleType = ListExpr.theEmptyList();
		}
		return ok;
	}
	
	/**
	 * Reads the first tuple of the relation list expression.
	 */
	public List<SecondoObject> readTupleValueFromLE(ListExpr pLE)
	{
		List<SecondoObject> result = new ArrayList<SecondoObject>();
		ListExpr tupleValues = pLE.second().first();
		ListExpr nextValue;
		//Reporter.debug("Relation.readTupleValueFromLE: tupleValues is " + tupleValues.toString());
		int No = 0;

		if(tupleValues.listLength() == this.relTypeInfo.getSize())  // correct tuplelength
		{
			while(tupleValues.listLength()>0)
			{
				nextValue = tupleValues.first();
				//Reporter.debug("Relation.readTupleValueFromLE: nextValue is " + nextValue.toString());
								
				SecondoObject SO = new SecondoObject(IDManager.getNextID());
				ListExpr Type = ListExpr.symbolAtom(this.relTypeInfo.get(No).Type);
				SO.fromList(ListExpr.twoElemList(Type, nextValue));
				String aName = computeObjectName(this.relTypeInfo.get(No).Name, 
												 this.relTypeInfo.get(No).Type, 
												 nextValue);
				SO.setName(Name+"::"+aName);
				
				result.add(SO);
				//Reporter.debug("Relation.readTupleValueFromLE: new SO is " + SO.toString());
				
				tupleValues = tupleValues.rest();
				No++;
			}
		}

		return result;
	}
	
	
	
	/** computes a short Name for a object */
	private String computeObjectName(String name,String type,ListExpr value)
	{
		int len = relTypeInfo.getMaxNameLength();
		String tmpname="";
		for(int i=0;i<len+1-name.length();i++)
			tmpname = tmpname+" ";
		tmpname += name+" ";
		
		String ValueString;
		if(!value.isAtom() && value.listLength()==1 && value.first().atomType()==ListExpr.TEXT_ATOM)
			value = value.first();
		
		if (!value.isAtom())
		{
			ValueString = type;
		}
		else
		{
			int atomType = value.atomType();
			switch (atomType){
				case ListExpr.REAL_ATOM : 
					ValueString=Double.toString(value.realValue()); 
					break;
				case ListExpr.STRING_ATOM : 
					ValueString= value.stringValue(); 
					break;
				case ListExpr.INT_ATOM : 
					ValueString = Integer.toString(value.intValue()); 
					break;
				case ListExpr.SYMBOL_ATOM : 
					ValueString = value.symbolValue(); 
					break;
				case ListExpr.BOOL_ATOM : 
					ValueString = Boolean.toString(value.boolValue()); 
					break;
				case ListExpr.TEXT_ATOM :  
					if(value.textLength()>48)
					{
						ValueString = "TEXT "+value.textLength()+" chars";
					}
					else
					{
						ValueString = value.textValue();
					}
					break;
				case ListExpr.NO_ATOM : 
					ValueString= type; 
					break;
				default : ValueString = "unknown type";
			}
		}
		return tmpname+": "+ValueString;
		//return tmpname;
	}
	
	
	
	/** check if SO contains a relation */
	public static boolean isRelation(SecondoObject SO)
	{
		return RelationTypeInfo.isRelation(SO.toListExpr());
	}
	
	
	/** return the SecondoObject on given position
	 * both numbers are started with 0
	 */
	public SecondoObject getSecondoObject(int TupleNumber,int ObjectNumber)
	{
		if (!initialized)
			return null;
		int index = relTypeInfo.getSize()*TupleNumber+ObjectNumber;
		if(index<0 |  index>SecondoObjects.size())
			return null;
		else
			return (SecondoObject) SecondoObjects.get(index);
	}
	
	
	/** return the Tuple on give Position */
	public SecondoObject[] getTupleAt(int index)
	{
		if(!initialized)
			return null;
		int startTuple = index*relTypeInfo.getSize();
		if(startTuple<0 || startTuple+relTypeInfo.getSize()>SecondoObjects.size())
			return null;
		
		SecondoObject[] Tuple = new SecondoObject[relTypeInfo.getSize()];
		for(int i=0;i<relTypeInfo.getSize();i++)
		{
			Tuple[i]  = (SecondoObject) SecondoObjects.get(i+startTuple);
		}
		return Tuple;
	}
	
	/** Removes the tuple by its ID */
	public void removeTupleByID(String pTupleId)
	{
		if(initialized)
		{
			int ti = this.TupleIDs.indexOf(pTupleId);
			Reporter.debug("Relation.removeTupleByID: tupleID "+ pTupleId + " has index " + ti);

			if(ti >= 0)
			{
				for(int j=0; j<relTypeInfo.getSize(); j++)
				{
					int soi = ti * relTypeInfo.getSize();
					Reporter.debug("Relation.removeTupleByID: secondoobject has index " + soi);
					Reporter.debug("Relation.removeTupleByID: secondoobjects.size is " + SecondoObjects.size());
					Reporter.debug("Relation.removeTupleByID: removing " + SecondoObjects.get(soi).toString());
					SecondoObjects.remove(soi);
					Reporter.debug("Relation.removeTupleByID:  XXXX" + SecondoObjects.toString() + "XXXXXXXXXXX");
				}
				TupleIDs.remove(ti);
			}
		}
	}
	
	
	/** 
	 * Set tuple on given position 
	 */
	public void setTupleAt(int pIndex, SecondoObject[] pTuple)
	{
		if(!initialized)
			return;
		
		int startTuple = pIndex*relTypeInfo.getSize();
		if(startTuple < 0 
				|| startTuple+relTypeInfo.getSize() > SecondoObjects.size() 
				|| pTuple.length != SecondoObjects.size())
			return;
		
		SecondoObject soNew;
		
		for(int i=0;i<relTypeInfo.getSize();i++)
		{
			soNew = pTuple[i];
			soNew.setID(SecondoObjects.get(startTuple + i).getID());
			SecondoObjects.set(startTuple + i, soNew);
		}
	}


	
	/** 
	 * Set SecondoObject on given position 
	 */
	public void setSecondoObject(int pTupleIndex, int pAttributeIndex, SecondoObject pAttribute)
	{
		if(!initialized)
			return;
		
		int objectIndex = pTupleIndex*relTypeInfo.getSize() + pAttributeIndex;
		if(objectIndex < 0 || objectIndex > SecondoObjects.size())
			return;
		
		SecondoObject so = pAttribute;
		so.setID(this.SecondoObjects.get(objectIndex).getID());
		SecondoObjects.set(objectIndex, so);
	}
	
	
	/** computes a Tuple and returns it
	public SecondoObject getTupleNo(int index)
	{
		SecondoObject[] Content = getTupleAt(index);
		if (Content==null)
			return null;
		// compute the value_list
		ListExpr Value,Last=null;
		if(Content.length==0)
			Value = ListExpr.theEmptyList();
		else
		{
			Value = ListExpr.oneElemList(Content[0].toListExpr().second());
			Last = Value;
		}
		ListExpr Next;
		for(int i=1;i<Content.length;i++)
		{
			Next = (Content[i].toListExpr().second());
			Last = ListExpr.append(Last,Next);
		}
		
		SecondoObject Tuple = new SecondoObject((ID)TupleIDs.get(index));
		Tuple.fromList(ListExpr.twoElemList(TupleType,Value));
		Tuple.setName(Name+" ["+index+"]");
		return Tuple;
	}
	*/
	
	/** returns the Relation */
	public SecondoObject getRelation()
	{
		return WholeRelation;
	}
	
	
	/** returns the object on index */
	public SecondoObject get(int index)
	{
		if(index<0 || index>SecondoObjects.size())
			return null;
		else
			return (SecondoObject) SecondoObjects.get(index);
	}
	
	
	/** returns the number of containing tuples ,
	 * if this Relation is not initialized -1 is returned
	 */
	public int getTupleCount()
	{
		if(!initialized)
			return -1;
		else
			return SecondoObjects.size()/relTypeInfo.getSize();
	}
	
	/* returns the number of objects in a tuple
	 * if this relation not initialized -1 is returned
	 */
	public int getTupleSize()
	{
		if (!initialized)
			return -1;
		else
			return relTypeInfo.getSize();
	}
	
	/* returns the number of all containing objects
	 * if this relation not initialized -1 is returned
	 */
	public int getSize()
	{
		if(!initialized)
			return -1;
		else
			return SecondoObjects.size();
	}
	
	
	public boolean isInitialized()
	{ 
		return initialized;
	}
	
	
	public String getID()
	{
		if(!initialized)
			return null;
		else
			return myID;
	}
	
	public String toString()
	{
		StringBuffer sb = new StringBuffer("[Relation]: ");
		sb.append(", Name: ").append(this.getName());
		sb.append(", attributeNames: ");
		for (String name : this.getAttributeNames())
		{
			sb.append(name).append(", ");
		}
		sb.append(", attributeTypes: ");
		for (String type : this.getAttributeTypes())
		{
			sb.append(type).append(", ");
		}
		return sb.toString();
	}
	
}
