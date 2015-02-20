package com.secondo.webgui.client.mainview;

import com.google.gwt.user.client.ui.HTML;
import com.google.gwt.user.client.ui.HasVerticalAlignment;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Image;
import com.google.gwt.user.client.ui.StackPanel;
import com.google.gwt.user.client.ui.VerticalPanel;

public class SimpleQueriesStackPanel extends StackPanel{

	private SimpleQueryPanel passesPanel;
	private SimpleQueryPanel passesThroughRegionPanel;
	private SimpleQueryPanel atinstantPanel;
	private SimpleQueryPanel deftimePanel;	

	private Image queryIcon = new Image("resources/images/check-icon.png");
	
	
	public SimpleQueriesStackPanel(){
		String passesHeader = getHeaderStringLevel2("passes", queryIcon);
		passesPanel = new SimpleQueryPanel(
				"Does the trip pass through ...(i.e. specified street, cardinal direction, speed tempo)?",
				"passes", "i.e. Baker St");
		this.add(passesPanel, passesHeader, true);	
		
		String passesThroughRegionHeader = getHeaderStringLevel2("passes through region", queryIcon);
		passesThroughRegionPanel = new SimpleQueryPanel(
				"Does the trip pass through region?", "passes", "region on the map");
		passesThroughRegionPanel.setWidth("100%");
		this.add(passesThroughRegionPanel, passesThroughRegionHeader, true);			

		String atinstantHeader = getHeaderStringLevel2("atinstant", queryIcon);
		atinstantPanel = new SimpleQueryPanel(
				"Through what does the trip pass at defined time?",
				"atinstant", "i.e. 2012-01-01-01:15");
		this.add(atinstantPanel, atinstantHeader, true);

		String deftimeHeader = getHeaderStringLevel2("deftime", queryIcon);
		deftimePanel = new SimpleQueryPanel(
				"Determine the time intervals when the trip was at ... (i.e. specified street, southeast, moderate tempo)",
				"deftime", "i.e. Baker St");
		this.add(deftimePanel, deftimeHeader, true);	
				
	}
	
	/**
	 * Gets a string representation of the header that includes an image and
	 * some text.
	 * 
	 * @param text
	 *            the header text
	 * @param image
	 *            the Image to add next to the header
	 * @return the header as a string
	 */
	private String getHeaderStringLevel2(String text, Image image) {
		// Add the image and text to a horizontal panel
		HorizontalPanel hPanel = new HorizontalPanel();
		hPanel.setSpacing(0);
		hPanel.setVerticalAlignment(HasVerticalAlignment.ALIGN_MIDDLE);
		image.setSize("16px", "16px");
		image.setWidth("20px");
		hPanel.add(image);
		HTML headerText = new HTML(text);
		headerText.getElement().setAttribute("float", "left");
		hPanel.add(headerText);
		hPanel.setStyleName("customizedStackPanel");

		// Return the HTML string for the panel
		return hPanel.getElement().getString();
	}

	public SimpleQueryPanel getPassesPanel() {
		return passesPanel;
	}

	public SimpleQueryPanel getAtinstantPanel() {
		return atinstantPanel;
	}
	
	public SimpleQueryPanel getDeftimePanel() {
		return deftimePanel;
	}

	public SimpleQueryPanel getPassesThroughRegionPanel() {
		return passesThroughRegionPanel;
	}
}
