/*
 * CentroidMatch.java
 *
 * Created on 29. September 2007, 23:34
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package movingregion;

import java.util.*;

/**
 *
 * @author java
 */
public class SteinerPointMatch extends Match
{
    int threshold;
    /** Creates a new instance of CentroidMatch */
    public SteinerPointMatch(Region source, Region target,double thresholdRel, boolean useFinalize)
    {
        
        super(source,target,"Steiner-Point Match "+ ((int)(thresholdRel*100))+ " %","this implements a Matching according to the Steiner Point");
        
        LineWA[][] tmp=new LineWA[source.getNrOfFaces()+target.getNrOfFaces()][];
        for(int i=0;i<source.getNrOfFaces();i++)
        {
            tmp[i]=source.getFace(i).getCycle().getLines();
        }
        for(int i=0;i<target.getNrOfFaces();i++)
        {
            tmp[i+source.getNrOfFaces()]=target.getFace(i).getCycle().getLines();
        }
        double greatestDist=TriRepUtil.getMaxDistance2(tmp);
        threshold=(int)(greatestDist*thresholdRel);
        System.out.println("HALLO"+greatestDist+" "+threshold);
        this.addMatch(source,target);
        this.matchFaces(source.getFaces(),target.getFaces());
        
        if(useFinalize)
            this.fertig();
        this.generateRatings();
    }
    public void matchFaces(Face[] faces1,Face[] faces2)
    {
        
        HashSet unmatched=new HashSet(faces1.length+faces2.length);
        for(int i=0;i< faces1.length;i++)
        {
            for(int j=0;j<faces2.length;j++)
            {
                
                double distance= getDistance(faces1[i].getCycle(),faces2[j].getCycle());
                if(distance<threshold)
                {
                    this.addMatch(faces1[i],faces2[j]);
                    this.addMatch(faces2[j],faces1[i]);
                    this.addMatch(faces1[i].getCycle(),faces2[j].getCycle());
                    this.addMatch(faces2[j].getCycle(),faces1[i].getCycle());
                }
            }
            unmatched.add(faces1[i]);
        }
        for(int i=0;i<faces2.length;i++)
        {
            unmatched.add(faces2[i]);
        }
        while(!unmatched.isEmpty())
        {
            Face next=(Face)(unmatched.iterator().next());
            unmatched.remove(next);
            RegionTreeNode[] matches=this.getMatches(next);
            if(matches!=null&&matches[0]!=null)
            {
                if(matches.length>1)
                {
                    int dimMatch=0;
                    for(int i=0;i<matches.length;i++)
                    {
                        unmatched.remove(matches[i]);
                        dimMatch+=((Face)matches[i]).getCycle().getChildren().length;
                    }
                    this.matchCHTNs(next.getHolesAndConcavities(),this.getTargetChildren(next));
                }
                else
                {
                    if(getMatches(matches[0]).length>1)
                    {
                        for(int i=0;i<getMatches(matches[0]).length;i++)
                        {
                            unmatched.remove(getMatches(matches[0])[i]);
                        }
                        this.matchCHTNs(((Face)matches[0]).getHolesAndConcavities(),this.getTargetChildren(matches[0]));
                    }
                    else
                    {
                        unmatched.remove(matches[0]);
                        this.matchCHTNs(next.getHolesAndConcavities(),this.getTargetChildren(next));
                    }
                }
            }
        }
    }
    public void matchCHTNs(ConvexHullTreeNode[] chtn1,ConvexHullTreeNode[] chtn2)
    {
        if(chtn1.length==0)
        {
            return;
        }
        HashSet unmatched=new HashSet(source.getNrOfFaces()+target.getNrOfFaces());
        for(int i=0;i< chtn1.length;i++)
        {
            for(int j=0;j<chtn2.length;j++)
            {
                
                double distance= getDistance(chtn1[i],chtn2[j]);
                if(distance<threshold)
                {
                    this.addMatch(chtn1[i],chtn2[j]);
                    this.addMatch(chtn2[j],chtn1[i]);
                    System.out.println("addMatch");
                }
            }
            unmatched.add(chtn1[i]);
        }
        for(int i=0;i<chtn2.length;i++)
        {
            unmatched.add(chtn2[i]);
        }
        while(!unmatched.isEmpty())
        {
            ConvexHullTreeNode next=(ConvexHullTreeNode)(unmatched.iterator().next());
            unmatched.remove(next);
            RegionTreeNode[] matches=this.getMatches(next);
            if(matches!=null&&matches[0]!=null)
            {
                if(matches.length>1)
                {
                    for(int i=0;i<matches.length;i++)
                    {
                        unmatched.remove(matches[i]);
                    }
                    this.matchCHTNs(next.getChildren(),this.getTargetChildren(next));
                }
                else
                {
                    if(getMatches(matches[0]).length>1)
                    {
                        for(int i=0;i<getMatches(matches[0]).length;i++)
                        {
                            unmatched.remove(getMatches(matches[0])[i]);
                        }
                        this.matchCHTNs(((ConvexHullTreeNode)matches[0]).getChildren(),this.getTargetChildren(matches[0]));
                    }
                    else
                    {
                        unmatched.remove(matches[0]);
                        this.matchCHTNs(next.getChildren(),((ConvexHullTreeNode)matches[0]).getChildren());
                    }
                }
            }
        }
    }
    
    public Face getBestMatch(Face source,Face[] targets)
    {
        double best=Double.MAX_VALUE;
        Face bestMatch=null;
        for(int i=0;i<targets.length;i++)
        {
            double overl=getDistance(source.getCycle(),targets[i].getCycle());
            if(overl<best)
            {
                bestMatch=targets[i];
                best=overl;
            }
        }
        return(bestMatch);
    }
    
     public ConvexHullTreeNode getBestMatch(ConvexHullTreeNode source,ConvexHullTreeNode[] targets)
    {
        double best=Double.MAX_VALUE;
        ConvexHullTreeNode bestMatch=null;
        for(int i=0;i<targets.length;i++)
        {
            double overl=getDistance(source,targets[i]);
            if(overl<best)
            {
                bestMatch=targets[i];
                best=overl;
            }
        }
        return(bestMatch);
    }
    
    public static double getDistance(ConvexHullTreeNode chtn1,ConvexHullTreeNode chtn2)
    {
        LineWA center1=chtn1.getSteinerPoint();
        LineWA center2=chtn2.getSteinerPoint();
        return(Math.sqrt((center1.x-center2.x)*(center1.x-center2.x)+(center1.y-center2.y)*(center1.y-center2.y)));
    }
}
