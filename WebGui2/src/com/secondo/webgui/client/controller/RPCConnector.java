package com.secondo.webgui.client.controller;

import java.util.ArrayList;

import org.gwtopenmaps.openlayers.client.Projection;
import org.gwtopenmaps.openlayers.client.geometry.Point;

import com.google.gwt.core.client.GWT;
import com.google.gwt.i18n.client.NumberFormat;
import com.google.gwt.thirdparty.javascript.jscomp.Result;
import com.google.gwt.user.client.Timer;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.rpc.AsyncCallback;
import com.google.gwt.user.client.ui.FormPanel;
import com.google.gwt.user.client.ui.PopupPanel;
import com.secondo.webgui.client.mainview.MainView;
import com.secondo.webgui.client.rpc.SecondoService;
import com.secondo.webgui.client.rpc.SecondoServiceAsync;
import com.secondo.webgui.shared.model.DataType;
import com.sun.java.swing.plaf.windows.resources.windows;

/**
 * This class is responsible for RPC-Calls to the server side of the
 * application.
 * 
 * @author Irina Russkaya
 */
public class RPCConnector {

	/** Service object for RPC-Calls to the server */
	private SecondoServiceAsync secondoService = (SecondoServiceAsync) GWT
			.create(SecondoService.class);
	private MainView mainView;
	private PopupPanel loadingPopup;
	private String currentCommand = "";
	private String commandForCount = "";
	private boolean countLoaded = false;
	private boolean countNeeded=false;

	/**
	 * The standard message displayed to the user when the server cannot be
	 * reached or returns an error.
	 */
	private static final String SERVER_ERROR = "An error occurred while "
			+ "attempting to contact the server. Please check your network "
			+ "connection and try again.";

	public RPCConnector() {

	}

	/*
	 * ###################### Methods with RPC-Calls to the Application-Server
	 * for getting server-side Data #########################################
	 */

	/**
	 * Starts an RPC call to the server to send the command from the
	 * commandpanel to secondo and get the result back
	 * 
	 * @param command
	 *            The command to be send to the secondo server
	 * @param mv
	 *            The main view object
	 * @param lp
	 *            The loading popup object
	 * */
	public void sendCommand(String command, String command2, MainView mv,
			PopupPanel lp) {

		this.mainView = mv;
		this.loadingPopup = lp;
		this.currentCommand = command;
		this.commandForCount = command2;
		countLoaded = false;
		if(!this.commandForCount.isEmpty()){
			this.countNeeded=true;
		}else{
			this.countNeeded=false;
		}

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {

				loadingPopup.hide();
				System.err.println("Query failed: " + caught.getMessage());
			}

			@Override
			public void onSuccess(String result) {

				// result is returned from secondoserviceimpl method
				if (result.startsWith("Error") || result.contains("error")) {
					pasteTextInResultInfoLabel("Error in executing query");

					loadingPopup.hide();

				} else {

					// put secondo data into the corresponding views
					setTextView(mainView, loadingPopup);
					setSymTrajInPlain(mainView, loadingPopup);

					// get datatype resultlist for map
					getDatatypeResultList(mainView, loadingPopup);

					// start timer to wait for data being loaded
					Timer timer = new Timer() {

						private int counter = 0;
						private int maxCount = 100;

						@Override
						public void run() {

							// If data is loaded or maxtime is reached close
							// loading animation and stop timer
							if (counter >= maxCount) {
								loadingPopup.hide();
								cancel();
								return;
							}
							// check if data in map and graphical view is loaded
							if (mainView.isMapTurnedOn()) {

								if (countLoaded) {
									counter = maxCount;
								}
								counter++;
							}

						}
					};
					timer.scheduleRepeating(500);
				}
			}
		};
		secondoService.sendCommand(command, callback);
	}

	/**
	 * Sends to Secondo a command for counting a loaded relation and sets the
	 * result value (int number of tuples) in info label
	 */
	private void executeCountCommandAndSetNumberOfTuples() {
		if (mainView.getOptionsTabPanel().isPatternMatchingIsInitiated()) {
			if(countNeeded){
			 setNumberOfTuplesInPatternMatchingResult(commandForCount);}
			
			countLoaded = true;	
			return;
		}
		if (mainView.getOptionsTabPanel().isSimpleQueryForPassesIsInitiated()) {
			if(countNeeded){
				 setNumberOfTuplesInSimpleQueryResultPasses(commandForCount,
				 mainView, loadingPopup);}			
			countLoaded = true;
			return;

		}
		if (mainView.getOptionsTabPanel()
				.isSimpleQueryForPassesTrhoughRegionsInitiated()) {
			if(countNeeded){
				setNumberOfTuplesInSimpleQueryResultPassesThrough(commandForCount, mainView, loadingPopup);
			}
			
			countLoaded = true;
			return;
		} else {
			if(countNeeded){
			setNumberOfTuplesInSampleRelation(commandForCount);}
			countLoaded = true;
			return;
		}
	}

	/**
	 * Sets the specified String in the result info label of pattern and simple
	 * query panel or displays it in a dialog box
	 * 
	 * @param text
	 *            The message to be displayed
	 */
	private void pasteTextInResultInfoLabel(String text) {
		if (mainView.getOptionsTabPanel().isSimpleQueryForPassesIsInitiated()) {
			mainView.getOptionsTabPanel().getSimpleQueriesStackPanel()
					.getPassesPanel().getResultInfoLabel().setText(text);			
		}
		if (mainView.getOptionsTabPanel().isPatternMatchingIsInitiated()) {
			mainView.getOptionsTabPanel()
					.setTextInResultOfPatternMatchingLabel(text);
		}
		if (mainView.getOptionsTabPanel()
				.isSimpleQueryForPassesTrhoughRegionsInitiated()) {
			mainView.getOptionsTabPanel().getSimpleQueriesStackPanel()
					.getPassesThroughRegionPanel().getResultInfoLabel()
					.setText(text);			
		} else {
			Window.alert(text);
		}
	}

	/**
	 * Starts an RPC call to the server, gets the formatted text result and
	 * displays it in the text view
	 * 
	 * @param mv
	 *            The main view object
	 * @param lp
	 *            The loading popup object
	 */
	public void setTextView(MainView mv, PopupPanel lp) {

		this.mainView = mv;
		this.loadingPopup = lp;

		AsyncCallback<ArrayList<String>> callback = new AsyncCallback<ArrayList<String>>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
				loadingPopup.hide();
				mainView.getMainheader().getTextViewOfTrajInDialog()
						.setDataLoaded(true);
			}

			@Override
			public void onSuccess(ArrayList<String> textResultList) {

				if (!textResultList.isEmpty()) {
					
					mainView.getMainheader().getTextViewOfTrajInDialog()
							.getResultList().add(textResultList);
					mainView.getMainheader().getTextViewOfTrajInDialog()
							.updateTextView();
				}
				// resultlist is empty
				else {
					mainView.getMainheader().getTextViewOfTrajInDialog()
							.setDataLoaded(true);					
				}
			}
		};
		secondoService.getFormattedResult(callback);
	}
	
	/**
	 * @param mv
	 * @param lp
	 */
	public void setSymTrajInPlain(MainView mv, PopupPanel lp) {

		this.mainView = mv;
		this.loadingPopup = lp;

		AsyncCallback<ArrayList<String>> callback = new AsyncCallback<ArrayList<String>>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
				loadingPopup.hide();
				mainView.getMainheader().getTextViewOfTrajInDialog()
						.setDataLoaded(true);
			}

			@Override
			public void onSuccess(ArrayList<String> textResultList) {

				if (!textResultList.isEmpty()) {
					System.out.println("resultlist is not empty");
					mainView.getMainheader().getTextViewOfSymTraj()
							.getResultList().add(textResultList);
					mainView.getMainheader().getTextViewOfSymTraj()
							.updateTextView();
				}
				// resultlist is empty
				else {
					mainView.getMainheader().getTextViewOfTrajInDialog()
							.setDataLoaded(true);				
				}
			}
		};
		secondoService.getFormattedResultForSymTraj(callback);
	}

	/**
	 * Starts an RPC call to the server to get a list of datatypes from the
	 * secondoresult and put it into resultLists
	 * 
	 * @param mv
	 *            The main view object
	 * @param lp
	 *            The loading popup object
	 * */
	public void getDatatypeResultList(MainView mv, PopupPanel lp) {

		this.mainView = mv;
		this.loadingPopup = lp;

		AsyncCallback<ArrayList<DataType>> callback = new AsyncCallback<ArrayList<DataType>>() {

			@Override
			public void onFailure(Throwable caught) {

				Window.alert(SERVER_ERROR);
				loadingPopup.hide();
				mainView.getMapView().setDataLoaded(true);
			}

			@Override
			public void onSuccess(ArrayList<DataType> result) {

				mainView.getMapView().getCurrentResultTypeList().clear();
				mainView.getMapView().clearControllers();

				if (!result.isEmpty()) {

					for (DataType datatype : result) {
						mainView.getMapView().getCurrentResultTypeList()
								.add(datatype);
					}
					// initialize data in views
					mainView.getMapView().initializeOverlays();

					// start timer to wait for data being initialized
					Timer timer = new Timer() {

						private int counter = 0;
						private int maxCount = 100;

						@Override
						public void run() {

							// If data is initialized start drawing of data
							if (counter >= maxCount) {
								// resizing + update drawing
								if (mainView.isMapTurnedOn()) {
									mainView.showMapView();
								}
								cancel();
								return;
							}
							if (mainView.getMapView().isDataInitialized()) {
								counter = maxCount;
							}
							counter++;
						}
					};
					timer.scheduleRepeating(500);

				}
				// resultlist is empty
				else {
					mainView.getMapView().setDataLoaded(true);
				}
				executeCountCommandAndSetNumberOfTuples();
			}
		};
		secondoService.getResultTypeList(callback);
	}

	/**
	 * Starts an RPC call to the server to write a string to a textfile
	 * 
	 * @param text
	 *            The text to be saved in the textfile
	 * @param filename
	 *            The name of the textfile
	 * */
	public void saveTextFile(String text, String filename) {
		
		AsyncCallback<Void> callback = new AsyncCallback<Void>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(Void result) {

				String url = GWT.getModuleBaseURL()
						+ "downloadService?fileName=" + "secondo-text.txt";
				Window.open(url, "_blank",
						"status=0,toolbar=0,menubar=0,location=0");

				System.out.println("########### result is saved in file");
			}
		};
		secondoService.saveTextFile(text, filename, callback);
	}

	/** Starts an RPC call to the server to reset the object counter to 1 */
	public void resetObjectCounter() {

		AsyncCallback<Void> callback = new AsyncCallback<Void>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(Void result) {
				// do nothing
			}
		};
		secondoService.resetObjectCounter(callback);
	}

	/**Starts an RPC call to the server to import a GPX file to SECONDO server
	 * @param nameOfUploadedFile The name of uploaded file 
	 * @param option The option for creating symbolic trajectory
	 * @param mv The main view
	 * @param lp The popup panel
	 */
	public void doGPXimport(final String nameOfUploadedFile, final int option,
			MainView mv, PopupPanel lp) {
		this.mainView = mv;
		this.loadingPopup = lp;

		final String sufix = nameOfUploadedFile.substring(
				nameOfUploadedFile.lastIndexOf("/") + 1,
				nameOfUploadedFile.lastIndexOf("."));
		final String relName = "Raw" + sufix;
		String command = "let " + relName + " = gpximport('"
				+ nameOfUploadedFile + "') consume";

		System.out.println("Command " + command);

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(String result) {
				if (result.contains("error")
						&& !result.contains("already used")) {
					Window.alert(result);
				} else {
					if (result.contains("already used")) {
						deleteOldRelation(relName, "", nameOfUploadedFile,
								option);
					} else {
						makeMPfromGPX(relName, nameOfUploadedFile, option);
					}

				}
			}
		};
		secondoService.sendCommandWithoutResult(command, callback);

	}

	/**
	 * @param startRelationName
	 * @param nameOfUploadedFile The name of uploaded file
	 * @param option
	 */
	public void makeMPfromGPX(final String startRelationName,
			final String nameOfUploadedFile, final int option) {
		final String sufix = nameOfUploadedFile.substring(
				nameOfUploadedFile.lastIndexOf("/") + 1,
				nameOfUploadedFile.lastIndexOf("."));
		final String resultRelationName = "MPfromGPX" + sufix;
		String command = "let "
				+ resultRelationName
				+ " = "
				+ startRelationName
				+ " feed extend[Trip: makepoint(.Lon, .Lat)]sortby[Time asc]approximate[Time, Trip, [const duration value (0 300000)]]";
		System.out.println("Command " + command);

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(String result) {
				if (result.contains("already used")) {
					deleteOldRelation(resultRelationName, startRelationName,
							nameOfUploadedFile, option);
				} else {
					makeRelationFromMP(resultRelationName, nameOfUploadedFile,
							option);
				}
			}
		};
		secondoService.sendCommandWithoutResult(command, callback);
	}

	public void makeRelationFromMP(final String startRelationName,
			final String nameOfUploadedFile, final int option) {
		final String sufix = nameOfUploadedFile.substring(
				nameOfUploadedFile.lastIndexOf("/") + 1,
				nameOfUploadedFile.lastIndexOf("."));
		final String resultRelationName = "MPfromGPXrelation" + sufix;
		String command = "let " + resultRelationName + " = "
				+ startRelationName
				+ " feed namedtransformstream[Trip] consume";
		System.out.println("Command " + command);

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(String result) {
				if (result.contains("already used")) {
					deleteOldRelation(resultRelationName, startRelationName,
							nameOfUploadedFile, option);

				} else {
					createSymTraj(resultRelationName, nameOfUploadedFile,
							option, mainView, loadingPopup);
				}
			}
		};
		secondoService.sendCommandWithoutResult(command, callback);

	}

	public void createSymTraj(final String startRelationName,
			final String nameOfUploadedFile, final int option, MainView mv,
			PopupPanel lp) {
		final String sufix = nameOfUploadedFile.substring(
				nameOfUploadedFile.lastIndexOf("/") + 1,
				nameOfUploadedFile.lastIndexOf("."));
		String resultRelationName = "";
		String command = "";
		// speed mode
		if (option == 0) {
			resultRelationName = "SymTrajWithSpeedMode" + sufix;
			command = "let "
					+ resultRelationName
					+ "= "
					+ startRelationName
					+ " feed addcounter[No, 1] projectextend[No, Trip; Traj: trajectory(.Trip), SymTraj: units(speed(gk(.Trip))) transformstream extend[Speed: the_unit(tolabel(getSpeedString(val(initial(.Elem)))), inst(initial(.Elem)), inst(final(.Elem)), TRUE, FALSE)] makemvalue[Speed]] consume";
			System.out.println("Command " + command);
		}

		// direction
		if (option == 1) {
			resultRelationName = "SymTrajWithDirection" + sufix;
			command = "let "
					+ resultRelationName
					+ "= "
					+ startRelationName
					+ " feed addcounter[No, 1] projectextend[No, Trip; Traj: trajectory(.Trip), SymTraj: units(direction("
					+ startRelationName
					+ " feed extract[Trip])) transformstream extend[Direction: the_unit(tolabel(getDirectionString(val(initial(.Elem)))), inst(initial(.Elem)), inst(final(.Elem)), TRUE, FALSE)] makemvalue[Direction]] consume";
			System.out.println("Command " + command);
		}

		// distance
		if (option == 2) {
			Point point = mv.getMapView().getMyLocation();
			point.transform(new Projection("EPSG:900913"), new Projection(
					"EPSG:4326"));
			double lat = point.getX();
			double lon = point.getY();
			resultRelationName = "SymTrajWithDistance" + sufix;
			command = "let "
					+ resultRelationName
					+ "= "
					+ startRelationName
					+ " feed addcounter[No, 1] projectextend[No, Trip; Traj: trajectory(.Trip), SymTraj: units(distance(gk(.Trip), gk(point ( "
					+ lon
					+ " "
					+ lat
					+ " )))) transformstream extend[Distance: the_unit(tolabel(getDistanceString(val(initial(.Elem)))), inst(initial(.Elem)), inst(final(.Elem)), TRUE, FALSE)] makemvalue[Distance]] consume";
			System.out.println("Command " + command);
		}

		// administrative districts
		if (option == 3) {
			resultRelationName = "SymTrajWithAdminDistricts" + sufix;
			command = "let "
					+ resultRelationName
					+ "= "
					+ " AdminDistrictsGermany feed "
					+ startRelationName
					+ " feed addcounter[No, 1] itSpatialJoin[Gebiet, Trip] projectextend[No, Trip, KName; Traj: trajectory(.Trip), Pieces: .Trip at .Gebiet] filter[not(isempty(deftime(.Pieces)))] projectextendstream[No, Trip, Traj, KName; Time: components(deftime(.Pieces))] extend[Mintime: minimum(.Time), U: the_unit(tolabel(.KName), minimum(.Time), maximum(.Time), TRUE, FALSE)] sortby[No, Mintime] groupby[No;Trip : group feed extract[Trip], Traj : group feed extract[Traj], SymTrip : group feed makemvalue[U]] consume";
			System.out.println("Command " + command);
		}

		final String resultRelation = resultRelationName;
		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(String result) {
				if (result.contains("already used")) {
					deleteOldRelation(resultRelation, startRelationName,
							nameOfUploadedFile, option);

				} else {
					mainView.getOptionsTabPanel()
							.getSelectOptionsForExistingTrajectories()
							.addItem(resultRelation);
					Window.alert("Symbolic trajetory was scuccesfully created. On \"Try trajectory\" animate a newly created relation! ");
				}
			}

		};
		secondoService.sendCommandWithoutResult(command, callback);

	}

	public void deleteOldRelation(final String resultRelationName,
			final String startRelationName, final String nameOfUploadedFile,
			final int option) {

		final String command = "delete " + resultRelationName;
		System.out.println("Command " + command);

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(String result) {
				if (resultRelationName.contains("Raw")) {
					doGPXimport(nameOfUploadedFile, option, mainView,
							loadingPopup);
				}
				if (resultRelationName.contains("MPfromGPXrelation")) {

					makeRelationFromMP(startRelationName, nameOfUploadedFile,
							option);
				}
				if (resultRelationName.contains("MPfromGPX")
						&& !resultRelationName.contains("relation")) {

					makeMPfromGPX(startRelationName, nameOfUploadedFile, option);
				}
				if (resultRelationName.contains("SymTrajWith")) {
					createSymTraj(startRelationName, nameOfUploadedFile,
							option, mainView, loadingPopup);
				}

			}
		};
		secondoService.sendCommandWithoutResult(command, callback);
	}

	public void getCoordinateFromAddress(String command, MainView mv,
			PopupPanel lp) {
		this.mainView = mv;
		this.loadingPopup = lp;
		this.currentCommand = command;

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onSuccess(String result) {
				loadingPopup.hide();
				mainView.getMainheader().getLocationDialog()
						.setLabelForResult(result);

				if (!result.isEmpty() && !result.contains("failed")) {
					System.out.println(result);
					result = result.replace("(", "");
					result = result.replace(")", "");
					result = result.replace("point", "");
					result = result.replace("\n", "");
					result = result.trim();
					int end = result.indexOf(" ");
					String lon_str = result.substring(0, end);
					String lat_str = result.substring(result.indexOf(" ") + 1,
							result.length());
					double lon = NumberFormat.getDecimalFormat().parse(lon_str);
					double lat = NumberFormat.getDecimalFormat().parse(lat_str);
					System.out.println("Lon " + lon + " Lat " + lat);
					mainView.getMapView().centerOnMyLocation(lon, lat);

					// start timer to wait for data being initialized
					Timer timer = new Timer() {

						private int counter = 0;
						private int maxCount = 100;

						@Override
						public void run() {

							// If data is initialized start drawing of data
							if (counter >= maxCount) {
								// resizing + update drawing
								if (mainView.isMapTurnedOn()) {
									mainView.showMapView();
								}
								cancel();
								return;
							}
							if (mainView.getMapView().isDataInitialized()) {
								counter = maxCount;
							}
							counter++;
						}
					};
					timer.scheduleRepeating(500);

				}

			}

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
				loadingPopup.hide();
			}
		};
		secondoService.sendCommand(currentCommand, callback);
	}

	public void saveGPXfileToServer(final String filename, final FormPanel form) {

		if (filename.length() == 0) {
			Window.alert("Please select a file");
		}
		if (filename.endsWith(".gpx")) {
			AsyncCallback<Void> callback = new AsyncCallback<Void>() {

				@Override
				public void onFailure(Throwable caught) {
					Window.alert(SERVER_ERROR);

				}

				@Override
				public void onSuccess(Void result) {
					form.submit();
					String url = GWT.getModuleBaseURL()
							+ "uploadService?fileName=" + filename;
					Window.open(url, "_blank",
							"status=0,toolbar=0,menubar=0,location=0");
					Window.alert("File upload is successfull");
				}
			};

			secondoService.saveGPXfileToServer(filename, callback);

		} else {
			Window.alert("You can upload only gpx file");
		}
	}

	/**
	 * The method sends command to the Secondo server, adds it to command
	 * history and updates history.
	 * 
	 * @param command
	 *            to be sent to the server
	 * @param mv
	 * @param lp
	 */
	public void sendCommandAndUpdateHistory(String command, String command2,
			MainView mv, PopupPanel lp) {
		sendCommand(command, command2, mv, lp);
	}

	/**
	 * Sends command for count the opened relation to Secondo and updates label
	 * "number of tuples in relation" with returned int or error message
	 * 
	 * @param command
	 *            witch counts the opened relation
	 */
	public void setNumberOfTuplesInSampleRelation(String command) {

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);

			}

			@Override
			public void onSuccess(String result) {
				if (result.contains("error") || result.contains("Error")) {
					Window.alert(result);
				} else {
					result = result.replace("(", "");
					result = result.replace(")", "");
					result = result.replace("int", "");
					result = result.trim();

					mainView.getOptionsTabPanel()
							.getNumberOfTuplesInSampleRelation()
							.setText(result + " tuples");	
					mainView.getOptionsTabPanel().setTotalNumberOfTuplesInCurrentRelation(Integer.parseInt(result));
				}
				countLoaded = true;
			}
		};
		secondoService.sendCommand(command, callback);
	}

	/**
	 * Sets number of tuples and shows a result in pattern result part of
	 * options tabs
	 * 
	 * @param command
	 * @param mv
	 * @param lp
	 */
	public void setNumberOfTuplesInPatternMatchingResult(String command) {

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(String result) {
				loadingPopup.hide();
				
				if (result.contains("error") || result.contains("Error") || result.isEmpty()) {
					result = "Error in executing query";
				} else {
					result = result.replace("(", "");
					result = result.replace(")", "");
					result = result.replace("int", "");
					result = result.trim();
					
					mainView.getOptionsTabPanel()
							.setTotalNumberOfTuplesInPatternResult(
									Integer.parseInt(result));
				}
				mainView.getOptionsTabPanel().setTextInResultOfPatternMatchingLabel(
						"Result of pattern matching: " + result);
				mainView.getOptionsTabPanel().setPatternMatchingIsInitiated(false);

			}
		};
		secondoService.sendCommand(command, callback);
	}

	/**
	 * Sends command for count the matched tuples and sets the result value in
	 * the appropriate info label (for simple queries -- passes)
	 * 
	 * @param command
	 * @param mv
	 * @param lp
	 */
	public void setNumberOfTuplesInSimpleQueryResultPasses(String command,
			final MainView mv, final PopupPanel lp) {

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(String result) {
				lp.hide();
				if (result.contains("error") || result.contains("Error")) {
					result = "Error in executing query";
				}
				result = result.replace("(", "");
				result = result.replace(")", "");
				result = result.replace("int", "");
				result = result.trim();
				mv.getOptionsTabPanel().getSimpleQueriesStackPanel()
						.getPassesPanel().getResultInfoLabel()
						.setText("Result: " + result);
				mv.getOptionsTabPanel().setTotalNumberOfTuplesInPassesResult(Integer.parseInt(result));
				mv.getOptionsTabPanel().setSimpleQueryForPassesIsInitiated(
						false);
			}
		};
		secondoService.sendCommand(command, callback);
	}

	/**
	 * Sends command for count the matched tuples and sets the result value in
	 * the appropriate info label (for simple queries -- passes through region)
	 * 
	 * @param command
	 * @param mv
	 * @param lp
	 */
	public void setNumberOfTuplesInSimpleQueryResultPassesThrough(
			String command, final MainView mv, final PopupPanel lp) {

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(String result) {
				lp.hide();
				if (result.contains("error") || result.contains("Error")) {
					result = "Error in executing query";
				}
				result = result.replace("(", "");
				result = result.replace(")", "");
				result = result.replace("int", "");
				result = result.trim();
				mv.getOptionsTabPanel().getSimpleQueriesStackPanel()
						.getPassesThroughRegionPanel().getResultInfoLabel()
						.setText("Result: " + result);
				mv.getOptionsTabPanel().setTotalNumberOfTuplesInPassesThroughResult(Integer.parseInt(result));
				mv.getOptionsTabPanel()
						.setSimpleQueryForPassesTrhoughRegionsInitiated(false);
			}
		};
		secondoService.sendCommand(command, callback);
	}

	/**
	 * Serves simple queries for operators atinstant and deftime
	 * 
	 * @param command
	 * @param typeOfCommand
	 * @param mv
	 */
	public void sendSimpleQuery(final String command,
			final String typeOfCommand, MainView mv) {
		this.mainView = mv;

		AsyncCallback<String> callback = new AsyncCallback<String>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(String result) {

				if (typeOfCommand.equals("atinstant")) {
					if (result.contains("error")) {
						result = "Error in executing query";
					}
					if(result.length()<2000){
					mainView.getOptionsTabPanel().getSimpleQueriesStackPanel()
							.getAtinstantPanel().getResultInfoLabel()
							.setText(result);}
					else{
						Window.alert(result);
					}					
				}
				if (typeOfCommand.equals("deftime")) {
					if (result.contains("error")) {
						result = "Error in executing query";
					}
					
					if(result.length()<2000){
					mainView.getOptionsTabPanel().getSimpleQueriesStackPanel()
							.getDeftimePanel().getResultInfoLabel()
							.setText(result);}
					else{
						Window.alert(result);
					}
				}
			}
		};
		secondoService.sendCommand(command, callback);
	}

	/**
	 * Sends the specified message as email to support mail box
	 * 
	 * @param message
	 *            to be sent to support
	 */
	public void sendMailToSupport(String message) {

		AsyncCallback<Boolean> callback = new AsyncCallback<Boolean>() {

			@Override
			public void onFailure(Throwable caught) {
				Window.alert(SERVER_ERROR);
			}

			@Override
			public void onSuccess(Boolean result) {
				if (result) {
					Window.alert("Your message was successfully sent!");
					mainView.getMainheader().getSupportDialog()
							.getSupportDialogBox().hide();
					mainView.getMainheader().getSupportDialog()
							.cleanSupportDialogBox();
				} else {
					Window.alert("Exception while sending your mail");
				}
			}
		};
		secondoService.sendMail(message, callback);
	}
}
