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

package  viewer.update2.gui;

import java.awt.Color;
import java.awt.Component;
import java.awt.event.InputMethodListener;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.Rectangle;
import java.util.List;

import javax.swing.border.Border;
import javax.swing.BorderFactory;
import javax.swing.border.EmptyBorder;
import javax.swing.AbstractCellEditor;
//import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextArea;
import javax.swing.table.TableCellEditor;
import javax.swing.text.BadLocationException;
import javax.swing.text.DefaultHighlighter;
import javax.swing.text.Highlighter;

import tools.Reporter;

import viewer.update2.*;


/**
 * TableCellEditor for the attribute column of the relation table.
 */
public class ValueTableCellEditor extends AbstractCellEditor implements TableCellEditor
{
	private JTextArea textArea;
	private Border borderFocussed;
	private Highlighter hiliter;
	private DefaultHighlighter.DefaultHighlightPainter hilitePainter;
	//private JScrollPane scrollPane;
	
	/**
	 * Constructor
	 */
	public ValueTableCellEditor()
	{			
		this.textArea = new JTextArea();
		this.textArea.setEditable(true);
		this.textArea.setLineWrap(true);
		this.textArea.setWrapStyleWord(true);
		this.textArea.setForeground(Color.BLACK);
		//this.scrollPane = new JScrollPane(textArea, JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED, JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
		this.borderFocussed = BorderFactory.createLineBorder(Color.BLUE);
		this.hiliter = this.textArea.getHighlighter();
		this.hilitePainter = new DefaultHighlighter.DefaultHighlightPainter(Color.YELLOW);
				
		// insert line break on ENTER Key
		this.textArea.addKeyListener(new KeyAdapter(){
								public void keyPressed(KeyEvent event){
								if(event.getKeyCode()==KeyEvent.VK_ENTER){
									textArea.replaceSelection(System.getProperty("line.separator"));
                                    //textArea.setCaretPosition(textArea.getDocument().getLength()-1);

								}
								}
								});
	}
	
	
	/**
	 * Returns a component in which an attribute value can be edited.
	 */
	public Component getTableCellEditorComponent(
												   JTable pTable, Object pValue,
												   boolean pSelected,
												   int pRow, int pColumn) 
	{	
		// background
        if (pSelected || ((RelationTableModel)pTable.getModel()).isChanged(pRow, pColumn))
        {
            this.textArea.setBackground(new Color(210, 230, 255));
        }
        else
        {
            this.textArea.setBackground(Color.WHITE);
        }
		
		
		// border
		this.textArea.setBorder(BorderFactory.createCompoundBorder(this.borderFocussed,
                BorderFactory.createMatteBorder(1,5,1,1, this.textArea.getBackground())));

		
		// set text and correct row height according to textarea content
		int width = pTable.getColumnModel().getColumn(pColumn).getWidth();
		this.textArea.setSize(width, Short.MAX_VALUE);
		this.textArea.setText(pValue.toString());
		pTable.setRowHeight(pRow, this.textArea.getPreferredSize().height);
		
		
		// highlight search matches
		hiliter.removeAllHighlights();
		List<SearchHit> hits = ((RelationTableModel)pTable.getModel()).getSearchHits(pRow);
		if (hits != null && !hits.isEmpty())
		{
			for (SearchHit sh : hits) 
			{
				try 
				{
					hiliter.addHighlight(sh.getStart(), sh.getEnd(), this.hilitePainter);
				} catch (Exception e)
				{
					Reporter.debug("ValueTableCellEditor.getTableCellEditorComponent: highlighting failed ");
				}
			}
		}
		
		return this.textArea;
	}
		
	public Object getCellEditorValue()
	{
		return this.textArea.getText();
	}
	
	
	public Rectangle getOffset(int pTextPos)
	{
		try
		{
			Reporter.debug("ValueTableCellEditor.getOffset: textarea width is " + this.textArea.getPreferredSize().width);
			Reporter.debug("ValueTableCellEditor.getOffset: value is " + this.textArea.getText());
			return this.textArea.modelToView(pTextPos);
		}
		catch (BadLocationException e)
		{
			Reporter.showError("ValueTableCellEditor.getOffset: BadLocation " + pTextPos +  e.getMessage());
			return null;
		}
	}
	
	public void setCaretPosition(int pPosition)
	{
		Reporter.debug("ValueTableCellEditor.setCaretPosition: position is " + pPosition);
		if (pPosition >= 0 && pPosition < this.textArea.getText().length())
		{
			this.textArea.setCaretPosition(pPosition);
		}
	}
	
	
	public void setInputMethodListener(InputMethodListener pListener)
	{
		this.textArea.addInputMethodListener(pListener);
	}
}

