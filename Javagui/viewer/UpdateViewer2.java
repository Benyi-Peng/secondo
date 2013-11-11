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
import java.awt.FlowLayout;
import java.awt.GridLayout;

import java.util.ArrayList;
import java.util.List;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTabbedPane;
import javax.swing.JTextField;

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
	
	private JPanel searchPanel;
		
	private JButton load;
	
	private JButton clear;
	
	private JButton insert;
	
	private JButton delete;
	
	private JButton update;
	
	private JButton reset;
	
	private JButton undo;
	
	private JButton commit;
	
	private JButton search;

	private JLabel searchLabel;
	
	private JLabel searchResultLabel;

	private JTextField searchField;
	
	
	// Components to display the loaded relation set.
	private JTabbedPane tabbedPane;
	
	private List<RelationPanel> relationPanels;
		
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
		this.actionPanel.setLayout(new GridLayout(1, 8));
		this.load = new JButton("Load");
		this.load.addActionListener(this.controller);
		this.actionPanel.add(load);
		this.clear = new JButton("Clear");
		this.clear.addActionListener(this.controller);
		this.actionPanel.add(clear);
		this.insert = new JButton("Insert");
		this.insert.addActionListener(this.controller);
		this.actionPanel.add(this.insert);
		this.delete = new JButton("Delete");
		this.delete.addActionListener(this.controller);
		this.actionPanel.add(this.delete);
		this.update = new JButton("Update");
		this.update.addActionListener(this.controller);
		this.actionPanel.add(this.update);
		this.reset = new JButton("Reset");
		this.reset.addActionListener(this.controller);
		this.actionPanel.add(this.reset);
		this.undo = new JButton("Undo");
		this.undo.addActionListener(this.controller);
		this.actionPanel.add(this.undo);
		this.commit = new JButton("Commit");
		this.commit.addActionListener(this.controller); 
		this.actionPanel.add(this.commit);		
		this.add(actionPanel, BorderLayout.NORTH);
		
		// searchPanel
		this.searchPanel = new JPanel();
		this.searchPanel.setLayout(new FlowLayout());
		//this.searchLabel = new JLabel("Keyword: ");
		//this.searchPanel.add(this.searchLabel);
		this.searchField = new JTextField(20);
		this.searchPanel.add(this.searchField);
		this.search = new JButton("Search");
		this.search.addActionListener(this.controller); 
		this.searchPanel.add(this.search);
		this.searchResultLabel = new JLabel();
		this.searchPanel.add(this.searchResultLabel);
		this.add(searchPanel, BorderLayout.SOUTH);


		// tabbed pane
		this.relationPanels = new ArrayList<RelationPanel>();
		this.tabbedPane = new JTabbedPane(JTabbedPane.TOP, JTabbedPane.SCROLL_TAB_LAYOUT);
		this.add(tabbedPane, BorderLayout.CENTER);

		this.setSelectionMode(States.INITIAL);
	}
	
	/**
	 * Adds a tab with the specified relation to the tabbed pane.

	public void addRelationPanel(String pRelationName, ListExpr pRelationLe)
	{
		RelationPanel tab = new RelationPanel(pRelationName, this.controller, pRelationLe);
		this.tabbedPane.addTab(tab.getName(), tab);
	}
	 	 */

	/**
	 * Removes currently shown relation set from this viewer. 
	 * All information about these relations will be lost.	 	 
	 */
	public void clear() 
	{
		this.relationPanels.clear();
		this.tabbedPane.removeAll();
		this.validate();
		this.repaint();
	}
	
	/**
	 * Returns list index of currently active RelationPanel.
	public int getCurrentRelationIndex()
	{
		return this.tabbedPane.getSelectedIndex();
	}
	 */	
	
	
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
		RelationPanel result = null;
		for (RelationPanel relpanel : this.relationPanels)
		{
			if (relpanel.getName().equals(pRelName))
			{
				return relpanel;
			}
		}
		return result;
	}
	
	/**
	 * Returns text from Search field.
	 */
	public String getSearchKey()
	{
		return this.searchField.getText();
	}
	
	/**
	 * Creates or overwrites RelationPanel data with specified data.
	 */
	public boolean setRelationPanel(String pRelName, ListExpr pRelationLE)
	{
		RelationPanel rp = this.getRelationPanel(pRelName);
		if (rp == null){
			rp = new RelationPanel(pRelName, this.controller);
			this.relationPanels.add(rp);
		}
		return rp.createFromLE(pRelationLE);
	}
	
	
	/**
	 * Displays all loaded RelationPanels.
	 */
	public void showRelations()
	{
		for (RelationPanel rp : this.relationPanels)
		{
			tabbedPane.add(rp.getName(), rp);
		}
		
		this.validate();
		this.repaint();	
	}
	
	
	public void showSearchResult(List<SearchHit> pResult)
	{
		if (pResult == null || pResult.isEmpty())
		{
			this.searchResultLabel.setText("Found 0 matches.");
		}
		else
		{
			this.getCurrentRelationPanel().setSearchHits(pResult);
			this.searchResultLabel.setText("1 / " + pResult.size());
			SearchHit hit = pResult.get(0);
			this.getCurrentRelationPanel().relGoTo(hit.getRowIndex(), 2);
		}
	}
	
	
	/*
	 For each mode and state the viewer is in only certain operations and choices are possible.
	 This method assures only the actually allowed actions can be executed or chosen.	 
	 
	 */
	public void setSelectionMode(int selectMode) 
	{
		switch (selectMode) 
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
				search.setEnabled(false);
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
				search.setEnabled(true);
				getCurrentRelationPanel().setMode(selectMode);
				break;
			}
			case States.INSERT: 
			{
				insert.setBackground(Color.YELLOW);
				load.setEnabled(false);
				clear.setEnabled(false);
				insert.setEnabled(false);
				delete.setEnabled(false);
				update.setEnabled(false);
				reset.setEnabled(true);
				undo.setEnabled(true);
				commit.setEnabled(true);
				search.setEnabled(true);
				getCurrentRelationPanel().setMode(selectMode);
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
				update.setEnabled(false);
				reset.setEnabled(true);
				undo.setEnabled(true);
				commit.setEnabled(true);
				search.setEnabled(true);
				getCurrentRelationPanel().setMode(selectMode);
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
				update.setEnabled(true);
				reset.setEnabled(true);
				undo.setEnabled(true);
				commit.setEnabled(true);
				search.setEnabled(true);
				getCurrentRelationPanel().setMode(selectMode);
				break;
			}
			default:
				break;
		}
	}
	

	/*********************************************************
	 * Methods of SecondoViewer.
	 * Most of these methods are without function as the
	 * responsibility for loading and manipulation objects
	 * is with UpdateViewer2.
	 *********************************************************/
	
	/*
	 Method of SecondoViewer:
	 Get the name of this viewer.
	 The name is used in the menu of the MainWindow.
	 */
	public String getName() {
		return viewerName;
	}
	
	/*
	 Method of SecondoViewer:
	 Because this viewer shall not display objects others than relations loaded
	 by the viewer itself only false is returned.	 
	 */
	public boolean addObject(SecondoObject o) {
		return false;
	}
	
	/*
	 Method of SecondoViewer:
	 Because this viewer shall not display objects others than relations loaded
	 by the viewer itself no object shall be removed.	 
	 */
	public void removeObject(SecondoObject o) {
		
	}
	
	/*
	 Method of SecondoViewer:
	 Because this viewer shall not display objects others than relations loaded
	 by the viewer itself no objects shall be removed.	 
	 */
	public void removeAll() {
		
	}
	/*
	 Method of SecondoViewer:
	 Because this viewer shall not display objects others than relations loaded
	 by the viewer itself false is returned.	 
	 */
	public boolean canDisplay(SecondoObject o) {
		return false;
	}
	
	/*
	 Method of SecondoViewer:
	 Because this viewer shall not display objects others than relations loaded
	 by the viewer itself false is returned.	 
	 */
	public boolean isDisplayed(SecondoObject o) {
		return false;
	}
	
	/*
	 Method of SecondoViewer:
	 Because this viewer shall not display objects others than relations loaded
	 by the viewer itself false is returned.	 
	 */
	public boolean selectObject(SecondoObject O) {
		return false;
	}
	
	/*
	 Method of SecondoViewer:
	 Because all commands for this viewer can easily be accessed from the actionPanel
	 no MenuVector is built.	 
	 */
	public MenuVector getMenuVector() {
		return null;
	}
	
	/*
	 Method of SecondoViewer:
	 Because this viewer shall not display objects others than relations loaded
	 by the viewer itself 0 is returned.	 
	 */
	public double getDisplayQuality(SecondoObject SO) {
		return 0;
	}
 
}

