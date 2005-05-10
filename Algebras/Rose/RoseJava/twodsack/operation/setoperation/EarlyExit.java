/*
 * EarlyExit.java 2005-05-02
 *
 * Dirk Ansorge, FernUniversitaet Hagen
 *
 */

package twodsack.operation.setoperation;

/**
 * An EarlyExit exception is used to allow to drop out of certain operations implemented in SetOps. An 
 * EarlyExit flag must be set when calling one of these operations. Then, at some point during the execution of the
 * operation, it may be clear that the result will be true or false. An EarlyExit exception is then thrown immediately.
 * The calling method catches the exception and has the result much earlier than when waiting for the whole 
 * set operation to finish.
 */
public class EarlyExit extends Exception {
    /**
     * The 'empty' constructor.
     */
    EarlyExit() { super(); }

    /*
     * A constructor which allows to pass a message.
     */
    EarlyExit(String s) { super(s); }
}//end Exception EarlyExit
