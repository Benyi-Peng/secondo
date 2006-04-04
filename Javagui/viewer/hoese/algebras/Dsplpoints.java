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


package  viewer.hoese.algebras;

import java.awt.geom.*;
import java.awt.*;
import sj.lang.ListExpr;
import java.util.*;
import viewer.*;
import viewer.hoese.*;
import tools.Reporter;


/**
 * The displayclass of the points datatype (Rose algebra).
 */
public class Dsplpoints extends DisplayGraph {
  Vector points;
  Rectangle2D.Double bounds;
  private boolean defined;

  /**
   * Scans the representation of the points datatype and constructs the points Vector.
   * @param v the numeric value of a list of x- and y-coordinate
   * @see sj.lang.ListExpr
   * @see <a href="Dsplpointssrc.html#ScanValue">Source</a>
   */
  public void ScanValue (ListExpr value) {
    if(isUndefined(value)){
       err=false;
       defined = false;
       return;
    }
    defined = true;
    double koord[] = new double[2];
    points = new Vector(20, 20);
    while (!value.isEmpty()) {
      ListExpr v = value.first();
      if (v.listLength() != 2) {
        Reporter.writeError("Error: No correct points expression: 2 elements needed");
        err = true;
        return;
      }
      for (int koordindex = 0; koordindex < 2; koordindex++) {
        Double d = LEUtils.readNumeric(v.first());
        if (d == null) {
          err = true;
          return;
        }
        koord[koordindex] = d.doubleValue();
        v = v.rest();
      }
      if (!err) {
        if(!ProjectionManager.project(koord[0],koord[1],aPoint))
           Reporter.writeError("error in projection at coordinate ("+koord[0]+", "+koord[1]+")");
        else{
          points.add(new Point2D.Double(aPoint.x,aPoint.y));
        }
      }
      value = value.rest();
    }
  }

  /**
   * Init. the Dsplpoints instance.
   * @param type The symbol points
   * @param value The numeric value of a list of points.
   * @param qr queryresult to display output.
   * @see generic.QueryResult
   * @see sj.lang.ListExpr
   * @see <a href="Dsplpointssrc.html#init">Source</a>
   */
  public void init (ListExpr type, ListExpr value, QueryResult qr) {
    AttrName = type.symbolValue();
    ScanValue(value);
    if (err) {
      Reporter.writeError("Error in ListExpr :parsing aborted");
      qr.addEntry(new String("(" + AttrName + ": GA(points))"));
      defined = false;
      return;
    }
    qr.addEntry(this);
    ListIterator li = points.listIterator();
    bounds = null;
    while (li.hasNext()) {
      Point2D.Double p = ((Point2D.Double)li.next());
      if (bounds == null)
        bounds = new Rectangle2D.Double(p.getX(), p.getY(), 0, 0);
      else
        bounds = (Rectangle2D.Double)bounds.createUnion(new Rectangle2D.Double(p.getX(),
            p.getY(), 0, 0));
    }
    RenderObject = bounds;
  }

  /**
   * @return The boundingbox of all the points
   * @see <a href="Dsplpointssrc.html#getBounds">Source</a>
   */
  public Rectangle2D.Double getBounds () {
    return  bounds;
  }

  /**
   * Tests if a given position is contained in any of the points.
   * @param xpos The x-Position to test.
   * @param ypos The y-Position to test.
   * @param scalex The actual x-zoomfactor
   * @param scaley The actual y-zoomfactor
   * @return true if x-, ypos is contained in this points type
   * @see <a href="Dsplpointssrc.html#contains">Source</a>
   */
  public boolean contains (double xpos, double ypos, double scalex, double scaley) {
    if(!defined){
       return false;
    }
    boolean hit = false;
    ListIterator li = points.listIterator();
    double scale = Cat.getPointSize(renderAttribute,CurrentState.ActualTime)*0.7*scalex;  
    while (li.hasNext())
      hit |= (((Point2D.Double)li.next()).distance(xpos, ypos) <= scale);
    return  hit;
  }

  /**
   * Draws the included points by creating a Dsplpoint and calling its draw-method.
   * @param g The graphics context
   * @see <a href="Dsplpointssrc.html#draw">Source</a>
   */
  public void draw (Graphics g, double time) {
    if(!defined){
       return;
    }
    ListIterator li = points.listIterator();
    while (li.hasNext()) {
      Point2D.Double p = (Point2D.Double)li.next();
      new Dsplpoint(p, this).draw(g,time);
      //g2.fillOval((int)  p.getX()-3  ,(int) p.getY()-3,6,6);
    }
    drawLabel(g, bounds, time);
  }
}



