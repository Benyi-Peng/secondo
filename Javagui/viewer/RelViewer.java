package viewer;

import sj.lang.*;
import javax.swing.*;
import javax.swing.table.*;
import java.awt.*;
import java.util.Vector;
import gui.*;
import java.awt.event.*;

public class RelViewer extends SecondoViewer{

 private JComboBox ComboBox;
 private JScrollPane ScrollPane;
 private JTable CurrentTable;
 private Vector Tables;
 private JPanel dummy;  // to show nothing

 /** creates a new RelationViewer **/
 public RelViewer(){
   ComboBox = new JComboBox();
   ScrollPane = new JScrollPane();
   dummy = new JPanel(); 
   CurrentTable = null;
   Tables = new Vector(); 
   setLayout(new BorderLayout());
   add(ComboBox,BorderLayout.NORTH);
   add(ScrollPane,BorderLayout.CENTER);
   ComboBox.addActionListener(new ActionListener(){
       public void actionPerformed(ActionEvent evt){
          showSelectedObject();
       }});
 }


 /** shows the table of the selected object **/
 private void showSelectedObject(){
    int index = ComboBox.getSelectedIndex();
    if(index>=0){
      CurrentTable = (JTable)Tables.get(index);
      ScrollPane.setViewportView(CurrentTable);
    }
    else
      ScrollPane.setViewportView(dummy);
    
 }

 public  String getName(){
   return "RelationsViewer";
 }

 public boolean addObject(SecondoObject o){
   if (isDisplayed(o)){
      selectObject(o);
      return false;
   }
   else{
      JTable NTable = createTableFrom(o.toListExpr());
      if(NTable==null)
          return false;
      else{ 
         Tables.add(NTable);
         ComboBox.addItem(o.getName());
         selectObject(o);
         ScrollPane.setViewportView(NTable);
         return true;
       }
    }
 }

 

 /** created a new JTable from LE, if LE not a valid 
   * ListExpr for a relation null ist returned
   **/
 private JTable createTableFrom(ListExpr LE){
 boolean result = true;
 JTable NTable = null;
  if (LE.listLength()!=2)
     return null;
  else{
    ListExpr type = LE.first();
    ListExpr value = LE.second();
    // analyse type
    ListExpr maintype = type.first();
    if (type.listLength()!=2 || !maintype.isAtom() || maintype.atomType()!=ListExpr.SYMBOL_ATOM 
        || !maintype.symbolValue().equals("rel"))
       return null; // not a relation
    ListExpr tupletype = type.second();
    // analyse Tuple
    ListExpr TupleFirst=tupletype.first();
    if (tupletype.listLength()!=2 || !TupleFirst.isAtom() || 
         TupleFirst.atomType()!=ListExpr.SYMBOL_ATOM || !TupleFirst.symbolValue().equals("tuple"))
       return null; // not a tuple
    ListExpr TupleTypeValue = tupletype.second();
    // the table head
    String[] head=new String[TupleTypeValue.listLength()];
    for(int i=0;!TupleTypeValue.isEmpty()&&result;i++) {
        ListExpr TupleSubType = TupleTypeValue.first();
        if (TupleSubType.listLength()!=2)
           result = false;
        else{
           head[i] = TupleSubType.first().writeListExprToString();
          // remove comment below if Type is wanted
          // head[i] += "  "+ TupleSubType.second().writeListExprToString();  
        }
        TupleTypeValue = TupleTypeValue.rest();
    }

   if (result){
     // analyse the values
     ListExpr TupleValue;
     Vector V= new Vector();
     String[] row;
     int pos;
     ListExpr Elem; 
     while (!value.isEmpty()){
         TupleValue = value.first();
         row = new String[head.length];
         pos = 0;
         while(pos<head.length & !TupleValue.isEmpty()){
           Elem = TupleValue.first();
           if (Elem.isAtom() && Elem.atomType()==ListExpr.STRING_ATOM)
              row[pos] = Elem.stringValue();
           else
              row[pos] = TupleValue.first().writeListExprToString();
           pos++;
           TupleValue = TupleValue.rest();
          }
         V.add(row);
         value = value.rest();
     }
     
     String[][] TableDatas = new String[V.size()][head.length];
     for(int i=0;i<V.size();i++)
          TableDatas[i]=(String[]) V.get(i);

     NTable = new JTable(TableDatas,head);
    }
  }

  if(result) 
      return NTable;
   else 
      return null;
}

 /** returns the index in ComboBox of S,
   * if S not exists in ComboBox -1 is returned 
   */
 private int getIndexOf(String S){
   int count =  ComboBox.getItemCount();
   int pos = -1;
   for(int i=0;i<count;i++){
     if( ((String)ComboBox.getItemAt(i)).equals(S)) pos=i;
   }
   return pos;
 }

 /** removes a object from this viewer **/
 public void removeObject(SecondoObject o){
    int index = getIndexOf(o.getName());
    if(index>=0){
       ComboBox.removeItemAt(index);
       Tables.remove(index);
       showSelectedObject();
    }
 }

 public boolean canDisplay(SecondoObject o){
   ListExpr LE = o.toListExpr();
   if(LE.listLength()!=2)
      return false;
   else{
     LE = LE.first();
     if(LE.isAtom())
       return false;
     else{
       LE = LE.first();
       if(LE.isAtom() && LE.atomType()==ListExpr.SYMBOL_ATOM && LE.symbolValue().equals("rel"))
           return true;
       else
           return false;
     }
   }  
 }

 /** check if o displayed in the moment **/
 public boolean isDisplayed(SecondoObject o){
   return getIndexOf(o.getName())>=0;
 }

/** hightlighting of o **/
 public  boolean selectObject(SecondoObject O){
      int index = getIndexOf(O.getName());
      if (index<0)
         return false;
      else{
         ComboBox.setSelectedIndex(index);
         showSelectedObject();
         return true;
      }
 }

 /** get the MenuExtension for MainWindow
   *
   **/
 public MenuVector getMenuVector(){ return null;}


 /** set the Control for this viewer **/
 public void setViewerControl(ViewerControl VC){
      this.VC = VC;
 }

 protected ViewerControl VC=null;  // inform this Control if select/remove a Object





}


