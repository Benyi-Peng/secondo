/*
 * ConvexHullTreeTreeViewer.java
 *
 * Created on 10. Juli 2007, 23:44
 */

package movingregion;

import java.awt.*;
import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.event.*;
/**
 *
 * @author  java
 */
public class ConvexHullTreeTreeViewer extends JPanel implements TreeSelectionListener
{
    JTree myTree;
    JScrollPane Scroll;
    ConvexHullTreeViewer myParent;
    boolean enabled=true;
    
    /** Creates new form ConvexHullTreeTreeViewer */
    public ConvexHullTreeTreeViewer(ConvexHullTreeNode node,ConvexHullTreeViewer myParent)
    {
        
        this.myParent=myParent;
        DefaultMutableTreeNode root=createNode(node);
        myTree=new JTree(root);
        myTree.setCellRenderer(new ConvexHullTreeTreeViewerRenderer());
        myTree.getSelectionModel().addTreeSelectionListener(this);
        setLayout(new BorderLayout());
        Scroll=new JScrollPane( JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED );
        Scroll.setViewportView(myTree);
        add(Scroll,BorderLayout.CENTER);
    }
    
    public ConvexHullTreeTreeViewer(Region region,ConvexHullTreeViewer myParent)
    {
        this.myParent=myParent;
        DefaultMutableTreeNode root=createRegion(region);
        myTree=new JTree(root);
        myTree.setCellRenderer(new ConvexHullTreeTreeViewerRenderer());
        myTree.getSelectionModel().addTreeSelectionListener(this);
        setLayout(new BorderLayout());
        Scroll=new JScrollPane( JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED );
        Scroll.setViewportView(myTree);
        add(Scroll,BorderLayout.CENTER);
    }
    
    public void valueChanged( TreeSelectionEvent e )
    {
        if(enabled)
        {
            TreePath[] tmp=myTree.getSelectionPaths();
            if(tmp!=null)
            {
                RegionTreeNode[] res=new RegionTreeNode[tmp.length];
                for(int i=0;i<tmp.length;i++)
                {
//                    System.out.println(i+" "+tmp[i].getLastPathComponent());
                    Object test2=tmp[i].getLastPathComponent();
                    DefaultMutableTreeNode test=(DefaultMutableTreeNode)(test2);
                    res[i]=(RegionTreeNode)test.getUserObject();
//                    System.out.println("OK");
                }
                myParent.changeSelection(res);
            }
        }
    }
    
    
    public static DefaultMutableTreeNode createRegion(Region region)
    {
//        System.out.println("region mit "+region.getNrOfFaces()+" Faces");
        DefaultMutableTreeNode res=new DefaultMutableTreeNode();
        res.setUserObject(region);
        for(int i=0;i<region.getNrOfFaces();i++)
        {
            res.add(createFace(region.getFace(i)));
        }
        return(res);
        
    }
    
    public static DefaultMutableTreeNode createFace(Face face)
    {
//        System.out.println("CHN mit "+face.getCycle().getOutLine().length+" Ecken");
        DefaultMutableTreeNode res=new DefaultMutableTreeNode();
        res.setUserObject(face);
        res.add(createNode(face.getCycle()));
        for(int i=0;i<face.getNrOfHoles();i++)
        {
            res.add(createNode(face.getHole(i)));
        }
        return(res);
        
    }
    
    public static DefaultMutableTreeNode createNode(ConvexHullTreeNode node)
    {
//        System.out.println("CHN mit "+node.getOutLine().length+" Ecken");
        DefaultMutableTreeNode res=new DefaultMutableTreeNode();
        res.setUserObject(node);
        ConvexHullTreeNode[] tmp=node.getChildren();
        for(int i=0;i<tmp.length;i++)
        {
            res.add(createNode(tmp[i]));
        }
        return(res);
        
    }
    
    public void setActual(RegionTreeNode[] actual)
    {
        enabled=false;
        //System.out.println(myTree.getSelectionPath());
        myTree.clearSelection();
        if(actual!=null)
        {
            for(int i=0;i<actual.length;i++)
            {                
                for(int j=0;j<myTree.getRowCount();j++)
                {
                    
                    TreePath path=findTreeNode(actual[i]);
//                    System.out.println(path);
                    //myTree.expandPath(path);
                    myTree.addSelectionPath(path);
                    /*if(actual[i]!=null&&actual[i].equals(((DefaultMutableTreeNode)(myTree.getPathForRow(j).getLastPathComponent())).getUserObject()))
                    {
                        System.out.println("expand"+j+" of "+myTree.getRowCount());
                        
                        myTree.expandRow(j);
                        myTree.addSelectionRow(j);         
                        
                    }*/
                }
            }
        }
        enabled=true;
        
        
    }
    public TreePath findTreeNode(RegionTreeNode find)
    {
        return(findTreeNode(find,((DefaultMutableTreeNode)(myTree.getModel().getRoot()))));
    }
    
    public TreePath findTreeNode(RegionTreeNode find, DefaultMutableTreeNode treem)
    {        
        if(find==null)
        {
            return(null);
        }
        if(find.equals(treem.getUserObject()))
        {
            return(new TreePath(treem));
        }
        else
        {
            TreePath res=null;
            for(int i=0;i<treem.getChildCount();i++)
            {                
                res=findTreeNode(find,((DefaultMutableTreeNode)treem.getChildAt(i)));
                if(res!=null)
                {
                    Object[] res2=new Object[res.getPathCount()+1];
                    res2[0]=treem;
                    for (int j=0;j<res.getPathCount();j++)
                    {
                        res2[j+1]=res.getPath()[j];
                    }                    
                    return(new TreePath(res2));
                }
            }
            return(null);
        }
    }
    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents()
    {

        setLayout(new java.awt.GridLayout());

    }// </editor-fold>//GEN-END:initComponents
    
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    // End of variables declaration//GEN-END:variables
    
}
