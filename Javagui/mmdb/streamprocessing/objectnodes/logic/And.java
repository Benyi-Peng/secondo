package mmdb.streamprocessing.objectnodes.logic;

import mmdb.data.attributes.standard.AttributeBool;
import mmdb.error.streamprocessing.ParsingException;
import mmdb.streamprocessing.Node;
import mmdb.streamprocessing.parser.Environment;
import mmdb.streamprocessing.parser.NestedListProcessor;
import mmdb.streamprocessing.tools.ParserTools;
import sj.lang.ListExpr;

/**
 * Implementation of logical AND operator resembling the core operator.<br>
 * Checks if both boolean parameters are true.
 * 
 * @author Björn Clasen
 */
public class And extends LogicalOperator {

	/**
	 * @see mmdb.streamprocessing.Nodes#fromNL(ListExpr[], Environment)
	 */
	public static Node fromNL(ListExpr[] params, Environment environment)
			throws ParsingException {
		ParserTools.checkListElemCount(params, 2, And.class);
		Node node1 = NestedListProcessor.nlToNode(params[0], environment);
		Node node2 = NestedListProcessor.nlToNode(params[1], environment);
		return new And(node1, node2);
	}

	/**
	 * Constructor, called by fromNL(...)
	 * 
	 * @param input1
	 *            operator's first parameter
	 * @param input2
	 *            operator's second parameter
	 */
	public And(Node input1, Node input2) {
		super(input1, input2);
	}

	/**
	 * {@inheritDoc}
	 */
	protected boolean calcResult(AttributeBool inputBool1,
			AttributeBool inputBool2) {
		if (inputBool1 == null || inputBool2 == null) {
			return false;
		}

		return inputBool1.isValue() && inputBool2.isValue();
	}

}
