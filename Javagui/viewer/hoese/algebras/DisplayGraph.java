
package  viewer.hoese.algebras;

import  java.awt.geom.*;
import  java.awt.*;
import  viewer.*;
import viewer.hoese.*;
import  sj.lang.ListExpr;


/**
 * This class is the superclass of all grapical objects. Because it fully implements the DsplGraph
 * interface, the subclasses mustn�t do it.
 */
public class DisplayGraph extends DsplGeneric
    implements DsplGraph {
/** if an error occurs during creation this field will be true */
  protected boolean err = false;
/** The layer in which this object is drawn */
  protected Layer RefLayer;
/** The category of this object preset by the defaultcategory */   
  protected Category Cat = Category.getDefaultCat();
/** The shape that was drawn by this instance */  
  protected Shape RenderObject;
/** Point datatayes e.g. point,points need special treatment,therefore this flag need to be set */ 
  protected boolean ispointType = false;
/** The label text of this object */
  private String LabelText;
/** Position of the label */  
  private Point LabPosOffset = new Point(-30, -10);

  /**
   * 
   * @return true if it is a pointtype object
   * @see <a href="DisplayGraphsrc.html#isPointType">Source</a>
   */
  public boolean isPointType () {
    return  ispointType;
  }

  /**
   * 
   * @return The text of the label.
   * @see <a href="DisplayGraphsrc.html#getLabelText">Source</a>
   */
  public String getLabelText () {
    return  LabelText;
  }

  /**
   * Sets the label text
   * @param label the text for the label
   * @see <a href="DisplayGraphsrc.html#setLabelText">Source</a>
   */
  public void setLabelText (String label) {
    LabelText = label;
  }

  /**
   * 
   * @return The position of the label as a Point
   * @see <a href="DisplayGraphsrc.html#getLabPosOffset">Source</a>
   */
  public Point getLabPosOffset () {
    return  LabPosOffset;
  }

  /**
   * Sets the position of the label
   * @param pt The position for the label
   * @see <a href="DisplayGraphsrc.html#setLabPosOffset">Source</a>
   */
  public void setLabPosOffset (Point pt) {
    LabPosOffset = pt;
  }

  /**
   * 
   * @return The boundingbox of the drawn Shape 
   * @see <a href="DisplayGraphsrc.html#getBounds">Source</a>
   */
  public Rectangle2D.Double getBounds () {
    if (RenderObject == null)
      return  null; 
    else 
      return  (Rectangle2D.Double)RenderObject.getBounds2D();
  }

  /**
   * Subclasses overrides this method to design their own Shape-object.
   * @param af The actual transformation of the graphic context, in which the object will be placed
   * @return Shape to render
   * @see <a href="DisplayGraphsrc.html#getRenderObject">Source</a>
   */
  public Shape getRenderObject (AffineTransform af) {
    return  RenderObject;
  }

  /**
   * This method draws the RenderObject with ist viewattributes collected in the category
   * @param g The graphic context to draw in.
   * @see <a href="DisplayGraphsrc.html#draw">Source</a>
   */
  public void draw (Graphics g) {
    Shape sh;
    Graphics2D g2 = (Graphics2D)g;
    AffineTransform af2 = RefLayer.getProjection();
    Shape render = getRenderObject(af2);
    if (render == null)
      return;
    sh = af2.createTransformedShape(render);
    //	if (!selected)
    g2.setComposite(Cat.getAlphaStyle());
    g2.setStroke(Cat.getLineStroke());
    g2.setPaint(Cat.getFillStyle());
    //the anchor of the texture has to do with Position of renderObject
    if (ispointType) {
      if (Cat.getFillStyle() instanceof TexturePaint)           // that indicates PointStyle
        g2.setPaint(new TexturePaint(((TexturePaint)Cat.getFillStyle()).getImage(), 
            sh.getBounds2D()));
      else if (Cat.getFillStyle() instanceof GradientPaint)           // that indicates PointStyle
        g2.setPaint(new GradientPaint((float)sh.getBounds().getX(),(float)sh.getBounds().getY(),
          ((GradientPaint)Cat.getFillStyle()).getColor1(),(float)(sh.getBounds().getX()+sh.getBounds().getWidth()),
          (float)(sh.getBounds().getY()+sh.getBounds().getHeight()),((GradientPaint)Cat.getFillStyle()).getColor2(),false)); 
    }
    if (Cat.getFillStyle() != null)
      g2.fill(sh);
    //if (selected) g2.setColor(new Color(Cat.LineColor.getRGB() ^ Color.white.getRGB() ));
    //else 
    g2.setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER));
    Color aktLineColor = Cat.getLineColor();
    if (selected){
      aktLineColor = new Color(Color.white.getRGB() ^ Cat.getLineColor().getRGB());
      }
    g2.setColor(aktLineColor);
    if ((Cat.getLineWidth() > 0.0f) || (selected))
      g2.draw(sh);
    drawLabel(g2, render);
  }

  /**
   * The draw method for the label.
   * @param g  The graphic context to draw in.
   * @param ro  The Shape-object to witch this label belong.
   * @see <a href="DisplayGraphsrc.html#drawLabel">Source</a>
   */
  public void drawLabel (Graphics g, Shape ro) {
    if (LabelText == null)
      return;
    Graphics2D g2 = (Graphics2D)g;
    Rectangle2D.Double r = (Rectangle2D.Double)ro.getBounds2D();
    AffineTransform af2 = RefLayer.getProjection();
    Point2D.Double p = new Point2D.Double(r.getX() + r.getWidth()/2, r.getY()
        + r.getHeight()/2);
    af2.transform(p, p);
    if (selected) {
      Rectangle2D re = g2.getFont().getStringBounds(LabelText, g2.getFontRenderContext());
      g2.setPaint(new Color(255, 128, 255, 255));
      g2.fill3DRect((int)(p.getX() + LabPosOffset.getX()), (int)(p.getY() + 
          LabPosOffset.getY() + re.getY()), (int)re.getWidth(), (int)re.getHeight(), 
          true);
    }
    g2.setPaint(Cat.getLineColor());
    g2.drawString(LabelText, (float)(p.getX() + LabPosOffset.getX()), (float)(
        p.getY() + LabPosOffset.getY()));
  }

  /**
   * Sets the category of this object.
   * @param acat A category-object
   * @see <a href="DisplayGraphsrc.html#setCategory">Source</a>
   */
  public void setCategory (Category acat) {
    Cat = acat;
  }

  /**
   * 
   * @return The actual category of this instance.
   * @see <a href="DisplayGraphsrc.html#getCategory">Source</a>
   */
  public Category getCategory () {
    return  Cat;
  }

  /**
   * Sets the layer of this instance.
   * @param alayer A layer to which this graphic object belong.
   * @see <a href="DisplayGraphsrc.html#setLayer">Source</a>
   */
  public void setLayer (Layer alayer) {
    RefLayer = alayer;
  }

  /**
   * 
   * @return The layer of this object
   * @see <a href="DisplayGraphsrc.html#getLayer">Source</a>
   */
  public Layer getLayer () {
    return  RefLayer;
  }

  /**
   * Tests if a given position is contained in the RenderObject.
   * @param xpos The x-Position to test.
   * @param ypos The y-Position to test.
   * @param scalex The actual x-zoomfactor 
   * @param scaley The actual y-zoomfactor
   * @return true if x-, ypos is contained in this points type
   * @see <a href="DisplayGraphsrc.html#contains">Source</a>
   */
  public boolean contains (double xpos, double ypos, double scalex, double scaley) {
    //if (! isActual())  RenderObject =createRenderObject();
    if (RenderObject == null)
      return  false;
    return  RenderObject.contains(xpos, ypos);
  }
  /** The text representation of this object 
   * @see <a href="DisplayGraphsrc.html#toString">Source</a>
   */
  public String toString () {
    return  AttrName + ":" + Cat.getName();                   
  }

}



