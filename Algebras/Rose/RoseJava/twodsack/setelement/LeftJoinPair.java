/*
 * LeftJoinPair.java 2005-05-03
 *
 * Dirk Ansorge FernUniversitaet Hagen
 *
 */

package twodsack.setelement;

import twodsack.set.*;
import twodsack.setelement.datatype.*;
import twodsack.util.*;

/**
 * The LeftJoinPair implements a special pair of objects. Every instance of this class is a pair <code>({@link twodsack.setelement.Element}
 * x {@link twodsack.set.ElemMultiSet})</code>.
 * Typically, the first partner (i.e. the Element) is somehow related to the objects in the second partner (the ElemMultiSet).
 * Instances are most commonly generated by a set operation in {@link twodsack.operation.setoperation.SetOps} like <tt>leftOuterJoin()</tt>.
 * In there, a predicate expresses the relation between the Element
 * and the ElemMultiSet. The predicate itself is not stored inside of a LeftJoinPair.<p>
 * Usually, instances of this class are stored in a {@link twodsack.set.LeftJoinPairMultiSet}.
 */
public class LeftJoinPair implements ComparableMSE{
    /*
     * fields
     */
    /**
     * An object of type Element is stored in this field.
     */
    public Element element;


    /**
     * A set of objects, namely an ElemMultiSet, is stored in this field.
     */
    public ElemMultiSet elemSet;

    /*
     * constructors
     */
    /**
     * The 'empty' constructor.
     */
    public LeftJoinPair() {};


    /**
     * Constructs a new instance of LeftJoinPair and directly assigns the element and multi set.
     *
     * @param el is assigned to this.element
     * @param ell is assigned to this.elemSet
     */
    public LeftJoinPair(Element el, ElemMultiSet ell) {
	element = el;
	elemSet = ell;
    }

    /*
     * methods
     */
    /**
     * Returns a <i>deep</i> copy of <i>this</i>.
     * This means, that real copies of <tt>this.element</tt> and <tt>this.elemSet</tt> are constructed.
     *
     * @return the copy
     */
    public LeftJoinPair copy() {
	LeftJoinPair copy = new LeftJoinPair();
	copy.element = this.element.copy();
	copy.elemSet = this.elemSet.copy();

	return copy;
    }//end method copy


    /**
     * Returns one of {0, 1, -1} depending on the result of the compare method.<p>
     * The implementation of this method takes us of the <tt>compare()</tt> method for <tt>this.element</tt>. Hence, two instances of 
     * type LeftJoinPair are sorted only by their <tt>element</tt> fields and not by the <tt>elemSet</tt> fields.
     *
     * @param inO must be another object of type LeftJoinPair
     * @return one of {0, 1, -1} as int
     * @throws WrongTypeException if inO.element has not the same type as this.inO.element or if inO is not of type
     * LeftJoinPair
     */
    public int compare(ComparableMSE inO) throws WrongTypeException {
	//comment missing
	if (!(inO instanceof LeftJoinPair)) {
	    throw new WrongTypeException("Error in LeftJoinPair.compareTo: Expected type LeftJoinPair, but found "+inO.getClass()); }
	
	Element firstE = ((LeftJoinPair)inO).element;
	return this.element.compare(firstE);
    }//end method compareTo
	
}//end class LeftJoinPair
