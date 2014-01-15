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

package  viewer;

import gui.SecondoObject;
import gui.idmanager.ID;
import gui.idmanager.IDManager;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.GridLayout;
import java.awt.Image;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTabbedPane;
import javax.swing.JEditorPane;
import javax.swing.JScrollPane;
import javax.swing.text.html.HTMLDocument;
import javax.swing.text.html.HTMLEditorKit;

import sj.lang.ListExpr;
import tools.Reporter;

import viewer.update2.*;
import viewer.update2.gui.*;


/**
 * This viewer allows editing of multiple relations.
 * It can be used to update, insert and delete tuples.
 * Tuple values are displayed sequentially in order to faciliate
 * viewing and editing of long text values. 
 */
public class UpdateViewer2 extends SecondoViewer {

	private String viewerName = "UpdateViewer2";
	
	private JPanel actionPanel;
			
	private JButton load;
	
	private JButton clear;
	
	private JButton insert;
	
	private JButton delete;
	
	private JButton update;
	
	private JButton reset;
	
	private JButton undo;
	
	private JButton commit;
	
	private JButton format;
	
	
	// Components to display the loaded relation set.
	private JTabbedPane tabbedPane;
	
	private List<RelationPanel> relationPanels;
	
	private List<JEditorPane> formattedDocuments;
	
	private JScrollPane scpFormattedDocument;
		
	// the controller decides which action shall be taken next and listens to all buttons
	// for user-input
	private UpdateViewerController controller;
	
		
	
	/*
	 * Constructor.	 
	 */
	public UpdateViewer2() 
	{
		this.controller = new UpdateViewerController(this);
				
		this.setLayout(new BorderLayout());	
		
		// actionpanel
		this.actionPanel = new JPanel();
		this.actionPanel.setLayout(new GridLayout(1, 9));
		this.load = new JButton(UpdateViewerController.CMD_LOAD);
		this.load.addActionListener(this.controller);
		this.load.setToolTipText("Open Load Dialog");
		this.actionPanel.add(load);
		this.clear = new JButton(UpdateViewerController.CMD_CLEAR);
		this.clear.addActionListener(this.controller);
		this.clear.setToolTipText("Remove loaded relations from viewer");
		this.actionPanel.add(clear);
		this.insert = new JButton(UpdateViewerController.CMD_INSERT);
		this.insert.addActionListener(this.controller);
		this.insert.setToolTipText("Change to Insert Mode");
		this.actionPanel.add(this.insert);
		this.delete = new JButton(UpdateViewerController.CMD_DELETE);
		this.delete.addActionListener(this.controller);
		this.delete.setToolTipText("Change to Delete Mode");
		this.actionPanel.add(this.delete);
		this.update = new JButton(UpdateViewerController.CMD_UPDATE);
		this.update.addActionListener(this.controller);
		this.update.setToolTipText("Change to Update Mode");
		this.actionPanel.add(this.update);
		this.reset = new JButton(UpdateViewerController.CMD_RESET);
		this.reset.addActionListener(this.controller);
		this.reset.setToolTipText("Undo all uncommitted changes");
		this.actionPanel.add(this.reset);
		this.undo = new JButton(UpdateViewerController.CMD_UNDO);
		this.undo.addActionListener(this.controller);
		this.undo.setToolTipText("Undo last uncommited change (cell-wise)");
		this.actionPanel.add(this.undo);
		this.commit = new JButton(UpdateViewerController.CMD_COMMIT);
		this.commit.addActionListener(this.controller); 
		this.commit.setToolTipText("Save changes to database");
		this.actionPanel.add(this.commit);
		this.format = new JButton(UpdateViewerController.CMD_FORMAT);
		this.format.addActionListener(this.controller); 
		this.format.setToolTipText("View as formatted document");
		this.actionPanel.add(this.format);		
		this.add(actionPanel, BorderLayout.NORTH);

		// tabbed pane
		this.relationPanels = new ArrayList<RelationPanel>();
		this.tabbedPane = new JTabbedPane(JTabbedPane.TOP, JTabbedPane.SCROLL_TAB_LAYOUT);
		this.add(tabbedPane, BorderLayout.CENTER);
		
		// formatted document
		this.formattedDocuments = new ArrayList<JEditorPane>();
		this.scpFormattedDocument = new JScrollPane();
		
		this.setSelectionMode(States.INITIAL);
	}
	

	/**
	 * Removes currently shown relation set from this viewer. 
	 * All information about these relations will be lost.	 	 
	 */
	public void clear() 
	{
		this.relationPanels.clear();
		this.tabbedPane.removeAll();
		this.formattedDocuments.clear();
		
		this.validate();
		this.repaint();
	}

	
	/**
	 * Returns the currently active RelationPanel.
	 */
	public RelationPanel getCurrentRelationPanel()
	{
		RelationPanel result = null;
		int index = this.tabbedPane.getSelectedIndex();
		if (index >= 0 && index < this.relationPanels.size())
		{
			result = this.relationPanels.get(index);
		}
		return result;
	}
	

	/**
	 * Returns the RelationPanel with the Relation of the specified name.
	 */
	public RelationPanel getRelationPanel(String pRelName)
	{
		for (RelationPanel relpanel : this.relationPanels)
		{
			if (relpanel.getName().equals(pRelName))
			{
				return relpanel;
			}
		}
		return null;
	}
	
	/**
	 * Returns the RelationPanel at specified index (list index = tab index), null if index invalid.
	 */
	public RelationPanel getRelationPanel(int pIndex)
	{
		if (pIndex >= 0 && pIndex < this.relationPanels.size())
		{
			return this.relationPanels.get(pIndex);
		}
		return null;
	}
	
	/**
	 * Returns all RelationPanels (list order = tab order).
	 */
	public List<RelationPanel> getRelationPanels()
	{
		return this.relationPanels;
	}
	
	
	public boolean removeRelationPanel(int pIndex)
	{
		if (pIndex >= 0 && pIndex < this.relationPanels.size())
		{
			this.relationPanels.remove(pIndex);
			this.tabbedPane.remove(pIndex);
			return true;
		}
		return false;
	}
	
	/**
	 * Sets RelationPanel at specified index as active tab in tabbed pane.
	 * if index is valid.
	 */
	public void showRelationPanel(int pIndex)
	{
		Reporter.debug("UpdateViewer2.showCurrentRelationPanel: setting index to " + pIndex);
		if (pIndex >= 0 && pIndex < this.relationPanels.size())
		{
			this.tabbedPane.setSelectedIndex(pIndex);
		}
		this.revalidate();
	}
	
	/**
	 * Creates or overwrites RelationPanel data with specified data.
	 * @param pRelName original name of SecondoObject
	 * @param pRelationLE complete relation ListExpression
	 * @param isDirectQuery TRUE if relation is opened read-only, especially if loaded by direct query in command window.
	 */
	public boolean setRelationPanel(String pRelName, ListExpr pRelationLE, boolean pEditable)
	{
		RelationPanel rp = this.getRelationPanel(pRelName);
		if (rp == null)
		{
			rp = new RelationPanel(pRelName, this.controller);			
			this.relationPanels.add(rp);
			
			String tabtitle = rp.getName();
			if (tabtitle.length() > 30)
			{
				tabtitle = tabtitle.substring(0,29) + "...";
			}
			
			this.tabbedPane.addTab(tabtitle, null, rp, rp.getName());			
			
			if (!pEditable)
			{
				Reporter.debug("UpdateViewer2.setRelationPanel: index=" + this.relationPanels.indexOf(this.getRelationPanel(rp.getName())));
				int index = this.relationPanels.indexOf(this.getRelationPanel(rp.getName()));
				this.tabbedPane.setComponentAt(index,
												  new ButtonTabComponent(index, tabtitle, this.controller));
			}
		}

		return rp.createFromLE(pRelationLE, pEditable);
	}
	


	/*
	 * For each mode and state the viewer is in only certain operations and choices are possible.
	 * This method assures only the actually allowed actions can be executed or chosen.	 
	 */
	public void setSelectionMode(int pState)
	{
		switch (pState) 
		{
			case States.INITIAL: 
			{
				insert.setBackground(Color.LIGHT_GRAY);
				delete.setBackground(Color.LIGHT_GRAY);
				update.setBackground(Color.LIGHT_GRAY);
				load.setEnabled(true);
				clear.setEnabled(false);
				insert.setEnabled(false);
				delete.setEnabled(false);
				update.setEnabled(false);
				reset.setEnabled(false);
				undo.setEnabled(false);
				commit.setEnabled(false);
				format.setEnabled(false);
				break;
			}
			case States.LOADED: 
			{
				insert.setBackground(Color.LIGHT_GRAY);
				delete.setBackground(Color.LIGHT_GRAY);
				update.setBackground(Color.LIGHT_GRAY);
				load.setEnabled(true);
				clear.setEnabled(true);
				insert.setEnabled(true);
				update.setEnabled(true);
				delete.setEnabled(true);
				reset.setEnabled(false);
				undo.setEnabled(false);
				commit.setEnabled(false);
				format.setEnabled(true);
				format.setText(UpdateViewerController.CMD_FORMAT);
				format.setToolTipText("View as formatted document");
				for (RelationPanel rp : this.relationPanels)
				{
					rp.setState(pState);
				}
				break;
			}
			case States.INSERT: 
			{
				insert.setBackground(Color.YELLOW);
				load.setEnabled(false);
				clear.setEnabled(false);
				insert.setEnabled(true);
				update.setEnabled(false);
				delete.setEnabled(false);
				reset.setEnabled(true);
				undo.setEnabled(true);
				commit.setEnabled(true);
				format.setEnabled(true);
				this.getCurrentRelationPanel().setState(pState);
				break;
			}
			case States.DELETE: 
			{
				delete.setBackground(Color.YELLOW);
				load.setEnabled(false);
				clear.setEnabled(false);
				insert.setEnabled(false);
				update.setEnabled(false);
				delete.setEnabled(false);
				reset.setEnabled(true);
				undo.setEnabled(true);
				commit.setEnabled(true);
				for (RelationPanel rp : this.relationPanels)
				{
					rp.setState(pState);
				}
				break;
			}
			case States.UPDATE: 
			{
				update.setBackground(Color.YELLOW);
				load.setEnabled(false);
				clear.setEnabled(false);
				insert.setEnabled(false);
				update.setEnabled(false);
				delete.setEnabled(false);
				reset.setEnabled(true);
				undo.setEnabled(true);
				commit.setEnabled(true);
				format.setEnabled(true);
				for (RelationPanel rp : this.relationPanels)
				{
					rp.setState(pState);
				}
				break;
			}
			case States.FORMAT: 
			{
				format.setBackground(Color.YELLOW);
				load.setEnabled(true);
				clear.setEnabled(false);
				insert.setEnabled(false);
				update.setEnabled(false);
				delete.setEnabled(false);
				reset.setEnabled(false);
				undo.setEnabled(false);
				commit.setEnabled(false);
				format.setEnabled(true);
				format.setText(UpdateViewerController.CMD_EDIT);
				format.setToolTipText("Edit relations");			
				break;
			}
			default:
				break;
		}
	}
	
	/**
	 * Displays generated document files.
	 * @param pFiles list of file paths
	 * @param pContentType can be text/plain, text/html and text/rtf. 
	 */
	public void loadFormattedDocument(List<String> pFiles, String pContentType) throws IOException
	{
		this.formattedDocuments.clear();

		for (String path: pFiles)
		{
			/*
			URL url = new URL("file", "", path);
			JEditorPane editorPane = new JEditorPane();
			editorPane.setContentType(pContentType);
			editorPane.setPage(url);			
			editorPane.setEditable(false);
			*/
			
			// read formatted file
			FileReader fr = new FileReader(path);
			BufferedReader br = new BufferedReader(fr);
			StringBuffer sb = new StringBuffer();
			String zeile = "";
			while( (zeile = br.readLine()) != null )
			{
				sb.append(zeile);
			}
			br.close();
			String page = sb.toString();
			Reporter.debug("DocumentFormatter.format: page=" + page);
			
			
			JEditorPane editorPane = new JEditorPane();
			editorPane.setEditable(false);
			HTMLDocument htmlDoc = new HTMLDocument();
			HTMLEditorKit editorKit = new HTMLEditorKit();
			editorPane.setEditorKit(editorKit);
			editorPane.setSize(new Dimension(800,600));
			editorPane.setMinimumSize(new Dimension(800,600));
			editorPane.setMaximumSize(new Dimension(800,600));
			editorPane.setOpaque(true);
			editorPane.setText(page);
			//bg.add(editorPane, BorderLayout.CENTER);
			

			this.formattedDocuments.add(editorPane);
		}
	}
	
	/**
	 * Shows relations in different display modes: States.FORMAT, States.LOADED, States.LOADED_READ_ONLY.
	 */
	public void showRelations(int pState)
	{
		switch (pState)
		{
			case States.FORMAT:
			{
				this.remove(tabbedPane);
				/*
				JPanel panel = new JPanel(new GridLayout(this.formattedDocuments.size(), 1));
				panel.setSize(new Dimension(800,600));
				panel.setMinimumSize(new Dimension(800,600));
				panel.setMaximumSize(new Dimension(800,600));
				
				panel.add(this.formattedDocuments.get(0));
				this.add(panel);
				
				for (JEditorPane doc: this.formattedDocuments)
				{
					this.scpFormattedDocument.add(doc);
				}
				*/
				
				this.scpFormattedDocument.setViewportView(this.formattedDocuments.get(0));
				
				//this.scpFormattedDocument.setSize(new Dimension(100,100));
				//this.scpFormattedDocument.setMaximumSize(new Dimension(100,100));
				//this.scpFormattedDocument.setViewportView(panel);
				
				this.add(scpFormattedDocument, BorderLayout.CENTER);
				
				break;
			}
			case States.LOADED:
			case States.LOADED_READ_ONLY:
			{
				this.remove(scpFormattedDocument);
				this.add(tabbedPane, BorderLayout.CENTER);
				break;
			}
		}
		this.validate();
		this.repaint();	
	}
	

	/*********************************************************
	 * Methods of SecondoViewer
	 *********************************************************/
	
	/**
	 * Method of SecondoViewer:
	 * Get the name of this viewer.
	 * The name is used in the menu of the MainWindow.
	 */
	@Override
	public String getName() 
	{
		return viewerName;
	}
	
	/**
	 * Method of SecondoViewer:
	 * Add new relation panel to display specified relation SecondoObject
	 * is SecondoObject is relation and not yet displayed.
	 */
	@Override
	public boolean addObject(SecondoObject so) 
	{
		if (this.canDisplay(so) && !this.isDisplayed(so))
		{
			this.setRelationPanel(so.getName(), so.toListExpr(), false);
			this.showRelations(States.LOADED);
			this.setSelectionMode(States.LOADED);
			RelationPanel rp = getRelationPanel(so.getName());
			if (rp != null)
			{
				int index = this.relationPanels.indexOf(rp);
				this.tabbedPane.setSelectedIndex(index);
			}
			return true;
		}
		return false;
	}
	
	/**
	 * Method of SecondoViewer
	 */
	@Override
	public void removeObject(SecondoObject so) 
	{
		RelationPanel rp = getRelationPanel(so.getName());
		if (rp != null)
		{
			int index = this.relationPanels.indexOf(rp);
			this.removeRelationPanel(index);
		}
	}
	
	/*
	 * Method of SecondoViewer.
	 * Remove all displayed relations from viewer.
	 */
	@Override
	public void removeAll() 
	{
		this.clear();
	}
		
	/*
	 * Method of SecondoViewer:
	 * Returns true if specified SecondoObject is a relation.	 
	 */
	public boolean canDisplay(SecondoObject so)
	{
		ListExpr le = so.toListExpr();
		Reporter.debug("UpdateViewer2.canDisplay: full ListExpr is " + le.toString());
		
		if (le.listLength() >= 2 && !le.first().isAtom())
		{
			le = le.first();
			//Reporter.debug("UpdateViewer2.canDisplay: type ListExpr is " + le.toString());
			
			if (!le.isAtom() && !le.isEmpty() && le.first().isAtom())
			{
				String objectType = le.first().symbolValue();
				if (objectType.equals("rel")) // || objectType.equals("trel") || objectType.equals("mrel"))
				{
					return true;
				}
			}
		}
		return false;
	}
	
	
	/*
	 Method of SecondoViewer:
	 Because this viewer shall not display objects others than relations loaded
	 by the viewer itself false is returned.	 
	 */
	@Override
	public boolean isDisplayed(SecondoObject so) 
	{
		RelationPanel rp = getRelationPanel(so.getName());
		if (rp != null)
		{
			return true;
		}
		return false;
	}
	
	/*
	 Method of SecondoViewer:
	 Because this viewer shall not display objects others than relations loaded
	 by the viewer itself false is returned.	 
	 */
	@Override
	public boolean selectObject(SecondoObject so) 
	{
		return false;
	}

	/*
	* Method of SecondoViewer:
	* Because all commands for this viewer can be accessed from the actionPanel
	* no MenuVector is built.	 
	*/
	@Override
	public MenuVector getMenuVector() 
	{
		return null;
	}
	
	/*
	 * Method of SecondoViewer 
	 */
	@Override
	public double getDisplayQuality(SecondoObject so) 
	{
		if (this.canDisplay(so))
		{
			if (so.toListExpr().toString().contains("text"))
			{
				// optimized for relation with (long) text attributes
				return 1;
			}
			else
			{
				// other relations are displayed as well
				// but may use too much space
				// as attributes are displayed sequentially
				return 0.5;
			}
		}
		return 0;
	}
 
}

