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

package viewer;

import javax.swing.*;

public class MessageBox{

private MessageBox(){}; // we need no constructor


public static void showMessage(String Text){
  OptionPane.showMessageDialog(null,Text);
}

public static int showQuestion(String ASK){
  int res = OptionPane.showConfirmDialog(null,ASK,null,
                      JOptionPane.YES_NO_OPTION,JOptionPane.QUESTION_MESSAGE);

  if(res==JOptionPane.YES_OPTION)
      return YES;
  if(res==JOptionPane.NO_OPTION)
      return NO;
  return ERROR;
}

private static JOptionPane OptionPane = new JOptionPane();
public static final int YES = 0;
public static final int NO = 1;
public static final int ERROR = -1;

}
