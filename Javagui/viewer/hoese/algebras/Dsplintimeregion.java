

package  viewer.hoese.algebras;

import  java.awt.geom.*;
import  java.awt.*;
import  viewer.*;
import viewer.hoese.*;
import  sj.lang.ListExpr;
import  java.util.*;
import  javax.swing.*;


/**
 * A displayclass for the intimeregion-type (spatiotemp algebra), 2D with TimePanel
 */
public class Dsplintimeregion extends Dsplregion
    implements Timed {
  Interval TimeBounds;

  /** A method of the Timed-Interface
   * 
   * @return the global time boundaries [min..max] this instance is defined at
   * @see <a href="Dsplintimeregionsrc.html#getTimebounds">Source</a>
   */
  public Interval getTimeBounds () {
    return  TimeBounds;
  }

  /**
   * A method of the Timed-Interface to render the content of the TimePanel
   * @param PixelTime pixel per hour
   * @return A JPanel component with the renderer
   * @see <a href="Dsplintimeregionsrc.html#getTimeRenderer">Source</a>
   */
  public JPanel getTimeRenderer (double PixelTime) {
    int start = 0;
    JLabel label = new JLabel("|"+LEUtils.convertTimeToString(TimeBounds.getStart()).substring(11, 
        16), JLabel.LEFT);
    label.setBounds(start, 15, 100, 15);
    label.setVerticalTextPosition(JLabel.CENTER);
    label.setHorizontalTextPosition(JLabel.RIGHT);
    JPanel jp = new JPanel(null);
    jp.setPreferredSize(new Dimension(100, 25));
    jp.add(label);
    //Add labels to the JPanel. 
    return  jp;
  }

  /**
   * Scans the representation of an instant datatype 
   * @param v An instant value
   * @see sj.lang.ListExpr
   * @see <a href="Dsplintimeregionsrc.html#ScanValue">Source</a>
   */
  public void ScanValue (ListExpr v) {
    Double d;
    //System.out.println(v.writeListExprToString());
    if (v.listLength() != 2) {                  //perhaps changes later
      System.out.println("Error: No correct intimeregion expression: 2 elements needed");
      err = true;
      return;
    }
    d = LEUtils.readInstant(v.first());
    if (d == null) {
      err = true;
      return;
    }
    TimeBounds = new Interval(d.doubleValue(), d.doubleValue(), true, true);
    super.ScanValue(v.second());
  }

  /**
   * This instance is only visible at its defined time.
   * @param at The actual Transformation, used to calculate the correct size.
   * @return Region Shape if ActualTime == defined time
   * @see <a href="Dsplintimeregionsrc.html#getRenderObject">Source</a>
   */
  public Shape getRenderObject (AffineTransform at) {
    double t = RefLayer.getActualTime();
    if (Math.abs(t - TimeBounds.getStart()) < 0.000001)
      return  super.getRenderObject(at); 
    else 
      return  null;
  }

  /**
   * Init. the Dsplintimeregon instance.
   * @param type The symbol intimeregion
   * @param value The value of an instant and a region
   * @param qr queryresult to display output.
   * @see generic.QueryResult
   * @see sj.lang.ListExpr
   * @see <a href="Dsplintimeregionsrc.html#init">Source</a>
   */
  public void init (ListExpr type, ListExpr value, QueryResult qr) {
    AttrName = type.symbolValue();
    ScanValue(value);
    if (err) {
      System.out.println("Error in ListExpr :parsing aborted");
      qr.addEntry(new String("(" + AttrName + ": GTA(IntimeRegion))"));
      return;
    } 
    else 
      qr.addEntry(this);
    RenderObject = areas;
  }

  /** A method of the Timed-Interface
   * @return The Vector representation of the time intervals this instance is defined at 
   * @see <a href="Dsplintimeregionsrc.html#getIntervals">Source</a>
   */
  public Vector getIntervals(){
    Vector v=new Vector(1,0);
    v.add(TimeBounds);
    return v;
    } 
}



