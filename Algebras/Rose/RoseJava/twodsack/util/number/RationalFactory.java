package twodsack.util.number;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.*;

public class RationalFactory {
    //This class is used for constructing numeric values.
    //It contains a set of methods called constRational()
    //which call constructors of implementations of the
    //abstract class Rational. At least two implementations
    //are provided, namely
    // - RationalBigInteger
    // - RationalDouble
    //
    //The implementation used is stored in the variable
    //RATIONAL_CLASS which must set using the method
    //setClass().
    //
    //Another variable PRECISE holds the user-selected value
    //TRUE or FALSE. It determines whether a precise but
    //slow computation is used when computing the following
    //predicates:
    //Mathset.linearly_dependent(Segment,Segment)
    //PointSeg_Ops.liesOn(Point,Segment)
    //PRECISE should be set using setPrecicision(). If not,
    //a warning is printed and TRUE is set.

    //members
    static private Class RATIONAL_CLASS = null;
    static private Boolean PRECISE = null;
    static private Constructor INTEGER_CONSTRUCTOR = null;
    static private Constructor DOUBLE_CONSTRUCTOR = null;
    static private Constructor RATIONAL_CONSTRUCTOR = null;
    static private Constructor NUM_DEN_CONSTRUCTOR = null;
    static private Object[] PARAM_VALUE_LIST_1 = new Object[1];
    static private Object[] PARAM_VALUE_LIST_2 = new Object[2];
    
    static public double DERIV_DOUBLE;
    static public double DERIV_DOUBLE_NEG;

    //methods
    static public void setClass(Class ratClass) {
	RATIONAL_CLASS = ratClass;
    }//end method setClass


    static public void setPrecision(boolean prec) {
	PRECISE = new Boolean(prec);
	Class [] classParam = { PRECISE.getClass() };
	Rational r = constRational(0);
	Object[] methParam = { PRECISE };
	try {
	    Method m = RATIONAL_CLASS.getMethod("setPrecision", classParam);
	    m.invoke(r,methParam);
	}//try
	catch (Exception e) {
	    System.out.println("RationalFactory.setPrecision: Problems with method setPrecise.");
	    e.printStackTrace();
	}//catch
	

    }//end method setPrecision


    static public void setClass(String ratClass) {
	try {
	    RATIONAL_CLASS = Class.forName(ratClass);
	}//try
	catch (Exception e) { throw new RationalClassNotExistentException("Error in RationalFactory: Class file "+ratClass+" was not found.");
	}//catch
    }//end method setClass

    /*
    static public void setClass_RationalBigInteger() {
	//written by Mirco 
	//replaces this by setClass(String)
	setClass(RationalBigInteger.class);
    }

    static public void setClass_RationalDoubles() {
	//written by Mirco
	//replace this by setClass(String)
	setClass(RationalDouble.class);
    }
    */

    static public Class readClass() {
	return RATIONAL_CLASS;
    }//end method getClass


    static public Rational readDeriv() {
	//if PRECISE = TRUE, it returns the derivation value from RATIONAL_CLASS
	//otherwise null is returned
	
	if (PRECISE == null) {
	    System.out.println("WARNING: PRECISE should be set using RationalFactory.setPrecision()!\n...automatic definition to prevent program termination: PRECISE = TRUE");
	    PRECISE = new Boolean(true);
	}//if
	if (PRECISE.booleanValue()) {
	    Object res;
	    Rational obj = constRational(0);
	    try {
		Field fi = RATIONAL_CLASS.getDeclaredField("deriv");
		res = fi.get(obj);
		//res = (Rational)RATIONAL_CLASS.getDeclaredField("deriv").get(RATIONAL_CLASS.newInstance());
		return (Rational)res;
	    }//try
	    catch (Exception e) { throw new NoDerivationValueFoundException("Error in RationalFactory: No derivation value was found. It must be implemented in chosen Rational class.");
	    }//catch
	}//if
	else { return null; }
    }//end method readDeriv


    static public double readDerivDouble() {
	if (RATIONAL_CLASS == null) {
	    throw new RationalClassUndefinedException("Error in RationalFactory: RATIONAL_CLASS must be set using setClass().");
	}//if
	try {
	    Field fi = RATIONAL_CLASS.getDeclaredField("DERIV_DOUBLE");
	    Double obj = null;
	    DERIV_DOUBLE = ((Double)fi.get(obj)).doubleValue();
	} catch (Exception e) { throw new NoDerivationValueFoundException("Error in RationalFactory: No DERIV_DOUBLE value was found in RATIONAL_CLASS ("+RATIONAL_CLASS+")."); }
	return DERIV_DOUBLE;
    }//end method readDerivDouble


    static public double readDerivDoubleNeg() {
	if (RATIONAL_CLASS == null) {
	    throw new RationalClassUndefinedException("Error in RationalFactory: RATIONAL_CLASS must be seut using setClass().");
	}//if
	try {
	    Field fi = RATIONAL_CLASS.getDeclaredField("DERIV_DOUBLE_NEG");
	    Double obj = null;
	    DERIV_DOUBLE_NEG = ((Double)fi.get(obj)).doubleValue();
	} catch (Exception e) { throw new NoDerivationValueFoundException("Error in RationalFactory: No DERIV_DOUBLE_NEG value was found in RATIONAL_CLASS ("+RATIONAL_CLASS+")."); }
	return DERIV_DOUBLE_NEG;
    }//end method readDerivDoubleNeg

    static public boolean readPrecise() {
	//returns the value of PRECISE
	if (PRECISE == null) {
	    System.out.println("WARNING: PRECISE should be set using RationalFactory.setPrecision()!\n...automatic definition to prevent program termination: PRECISE = TRUE");
	    PRECISE = new Boolean(true);
	}//if
	return PRECISE.booleanValue();
    }//end method readPrecise


    static public Rational constRational(int i) {
	if (RATIONAL_CLASS == null) {
	    throw new RationalClassUndefinedException("Error in RationalFactory: RATIONAL_CLASS must be set using setClass()");
	}//if
	else if (INTEGER_CONSTRUCTOR != null) {
	    //Object[] paramValueList = { new Integer(i) };
	    PARAM_VALUE_LIST_1[0] = new Integer(i);
	    Rational rat = null;
	    try {
		rat = (Rational)INTEGER_CONSTRUCTOR.newInstance(PARAM_VALUE_LIST_1);
	    }//try
	    catch (Exception e) {
		System.out.println("Error in RationalFactory.constRational(integer): Couldn't create a new instance for integer["+i+"].");
		e.printStackTrace();
		System.exit(0);
	    }//catch
	    return rat;
	}//else if

	else {
	    Class[] paramClassList = { int.class };
	    Object[] paramValueList = { new Integer(i) };
	    Rational rat = null;
	    try {
		INTEGER_CONSTRUCTOR = RATIONAL_CLASS.getConstructor(paramClassList);
		rat = (Rational)INTEGER_CONSTRUCTOR.newInstance(paramValueList);
	    }//try
	    catch (Exception e) { throw new RationalClassConstructorNotFoundException("Error in RationalFactory: constructor "+RATIONAL_CLASS+"(int) not found"); }
	    return rat;
	}//else
	    
    }//end method constRational


    static public Rational constRational(double d) {
	if (RATIONAL_CLASS == null) {
	    throw new RationalClassUndefinedException("Error in RationalFactory: RATIONAL_CLASS must be set using setClass()");
	}//if
	else if (DOUBLE_CONSTRUCTOR != null) {
	    //Object[] paramValueList = { new Double(d) };
	    PARAM_VALUE_LIST_1[0] = new Double(d);
	    Rational rat = null;
	    try {
		rat = (Rational)DOUBLE_CONSTRUCTOR.newInstance(PARAM_VALUE_LIST_1);
	    }//try
	    catch (Exception e) {
		System.out.println("Error in RationalFactory.constRational(double): Couldn't create a new instance for double["+d+"].");
		e.printStackTrace();
		System.exit(0);
	    }//catch
	    return rat;
	}//else if

	else {
	    Class[] paramClassList = { double.class };
	    Object[] paramValueList = { new Double(d) };
	    Rational rat = null;
	    try {
		DOUBLE_CONSTRUCTOR = RATIONAL_CLASS.getConstructor(paramClassList);
		rat = (Rational)DOUBLE_CONSTRUCTOR.newInstance(paramValueList);
	    }//try
	    catch (Exception e) { throw new RationalClassConstructorNotFoundException("Error in RationalFactory: constructor "+RATIONAL_CLASS+"(double) not found"); }
	    return rat;
	}//else
    }//end method constRational

    
    static public Rational constRational(Rational r) {
	if (RATIONAL_CLASS == null) {
	    throw new RationalClassUndefinedException("Error in RationalFactory: RATIONAL_CLASS must be set using setClass()");
	}//if
	else if (RATIONAL_CONSTRUCTOR != null) {
	    PARAM_VALUE_LIST_1[0]= r;
	    //Object[] paramValueList = { r };
	    Rational rat = null;
	    try {
		rat = (Rational)RATIONAL_CONSTRUCTOR.newInstance(PARAM_VALUE_LIST_1);
	    }//try
	    catch (Exception e) {
		System.out.println("Error in RationalFactory.constRational(Rational): Couldn't create a new instance for Rational["+r+"].");
		e.printStackTrace();
		System.exit(0);
	    }//catch
	    return rat;
	}//else if

	else {
	    Class[] paramClassList = { Rational.class };
	    Object[] paramValueList = { r };
	    Rational rat;
	    try {
		RATIONAL_CONSTRUCTOR = RATIONAL_CLASS.getConstructor(paramClassList);
		rat = (Rational)RATIONAL_CONSTRUCTOR.newInstance(paramValueList);
	    }//try
	    catch (Exception e) { throw new RationalClassConstructorNotFoundException("Error in RationalFactory: constructor "+RATIONAL_CLASS+"(Rational) not found"); }
	    return rat;
	}//else
    }//end method constRational

    
    static public Rational constRational(int num, int den) {
	if (RATIONAL_CLASS == null) {
	    throw new RationalClassUndefinedException("Error in RationalFactory: RATIONAL_CLASS must be set using setClass()");
	}//if
	
	else if (NUM_DEN_CONSTRUCTOR != null) {
	    //Object[] paramValueList = { new Integer(num), new Integer(den) };
	    PARAM_VALUE_LIST_2[0] = new Integer(num);
	    PARAM_VALUE_LIST_2[1] = new Integer(den);
	    Rational rat = null;
	    try {
		rat = (Rational)NUM_DEN_CONSTRUCTOR.newInstance(PARAM_VALUE_LIST_2);
	    }//try
	    catch (Exception e) {
		System.out.println("Error in RationalFactory.constRational(int,int): Couldn't create a new instance for int x int ["+num+", "+den+"].");
		e.printStackTrace();
		System.exit(0);
	    }//catch 
	    return rat;
	}//else if

	else {
	    Class[] paramClassList = { int.class, int.class };
	    Object[] paramValueList = { new Integer(num), new Integer(den) };
	    Rational rat;
	    try {
		NUM_DEN_CONSTRUCTOR = RATIONAL_CLASS.getConstructor(paramClassList);
		rat = (Rational)NUM_DEN_CONSTRUCTOR.newInstance(paramValueList);
	    }//try
	    catch (Exception e) { throw new RationalClassConstructorNotFoundException("Error in RationalFactory: constructor "+RATIONAL_CLASS+"(int int) not found"); }
	    return rat;
	}//else
    }//end method constRational


}//end class RationalFactory
