

package  viewer;

import  javax.swing.*;
import  java.awt.*;
import  java.awt.event.*;
import  java.net.URL;
import  java.io.*;
import  java.util.Properties;
import  java.util.Vector;
import  java.util.StringTokenizer;
import  sj.lang.ListExpr;
import  java.util.ListIterator;
import  javax.swing.event.*;
import  java.awt.geom.*;
import  javax.swing.border.*;
import  viewer.hoese.*;
import  gui.SecondoObject;

/**
 * this is a viewer for spatial and temporal spatial objects
 * but this viewer can display other query results
 */
public class HoeseViewer extends SecondoViewer {
  private MenuVector MenuExtension = new MenuVector();
  private boolean inAnApplet = false;
  private QueryListSelectionListener DoQuerySelection;
  private JSplitPane VisualPanel;
  private JSplitPane VisComPanel;
  private JPanel dummy = new JPanel();  // a place holder for temporal objects

  /** The top-right component for graph. objects in layer */
  public GraphWindow GraphDisplay;
  /** The left component for alphanumeric query-output */
  public TextWindow TextDisplay;
  private JPanel SpatioTempPanel;

  /** A link to the last query-result */
  public QueryResult CurrentQueryResult;
  private TimePanel TimeDisplay;

  /** The Zoomfactor of the GraphWindow */
  protected double ZoomFactor = 1;
  private AffineTransform ZoomTransform = new AffineTransform();
  private MouseListener SelectionControl;

  /** The time which is actually set in app. */
  public double ActualTime = 0;
  private Interval TimeBounds;
  private JScrollBar TimeSlider;

  /** The list of categories present in the app. */
  public Vector Cats;

  /** The context-panel where to made context setting */
  public ContextPanel context;

  /** The component which contains the togglebuttons for the layer at the left side of GraphWindow */
  public JPanel LayerSwitchBar;

  /** Rectangle of the WorldBoundingBox */
  public Rectangle2D.Double BBoxWC = new Rectangle2D.Double(0, 0, 0, 0);

  /** Rectangle of the Device-coordinates */
  public Rectangle BBoxDC = new Rectangle(0, 0, 0, 0);
  private int oldDL;

  /** True if a backimage is present */
  public boolean hasBackImage = false;
  private boolean isMouseSelected = false;
  private DsplGraph selGraphObj;
  private DsplBase selBaseObj;

  /** The projection from World (userspace) to Device-ccord. */
  public AffineTransform allProjection;

  /** The rectangle where device-coords are clipped. */
  public Rectangle ClipRect = null;
  private JLabel MouseKoordLabel;
  private JLabel actTimeLabel;
  private JScrollPane GeoScrollPane;
  private final String CONFIGURATION_FILE = "GBS.cfg";
  private final int NOT_ERROR_CODE = ServerErrorCodes.NOT_ERROR_CODE;

  private final boolean DEBUG_MODE = true;

  /** The main configuration parameter hash-table */
  public static Properties configuration;

  private javax.swing.JMenuBar jMenuBar1;
 /* File-Menu */
  private javax.swing.JMenu jMenu1;
  private javax.swing.JMenuItem jMenu_NewSession;
  private javax.swing.JMenuItem jMenu_OpenSession;
  private javax.swing.JMenuItem jMenu_CloseSession;
  private javax.swing.JMenuItem jMenu_SaveSession;
  private javax.swing.JMenuItem jMenu_SaveSessionAs;
  private javax.swing.JSeparator jSeparator1;
  private javax.swing.JMenuItem jMenu_Browse;
  private javax.swing.JMenuItem jMenu_Import;
  private javax.swing.JMenuItem jMenu_Export;
  private javax.swing.JSeparator jSeparator2;
  private javax.swing.JMenuItem jMenu_Exit;

 /* gui-menu */
  private javax.swing.JMenu jMenuGui;
  private javax.swing.JMenuItem delQueryMenu;
  private javax.swing.JMenuItem MINewCat;

  /** True if menu-entry 'automatic category' is selected */
  public javax.swing.JCheckBoxMenuItem isAutoCatMI;
  private javax.swing.JSeparator jSeparator5;
  private javax.swing.JMenuItem jMIShowCE;
  private javax.swing.JMenuItem MIQueryRep;

  /** context menu item */
  public javax.swing.JMenuItem MIsetKontext;
  private javax.swing.JMenuItem MILayerMgmt;
  private javax.swing.JMenu MenuObject;
  private javax.swing.JMenuItem MIHideObject;
  private javax.swing.JMenuItem MIShowObject;
  private javax.swing.JMenuItem MILabelAttr;
  private javax.swing.JMenuItem jMenuItem8;
  private javax.swing.JMenuItem MIMoveTop;
  private javax.swing.JMenuItem MIMoveUp;
  private javax.swing.JMenuItem MIMoveDown;
  private javax.swing.JMenuItem MIMoveBottom;
  private JMenuItem MIZoomOut;
  private JMenuItem MIZoomMinus;
  private JRadioButtonMenuItem RBMICustTI;
  private AbstractAction AAZoomOut;
  private AbstractAction AACatEdit;
  private AbstractAction AAViewCat;
  private AbstractAction AAContext;
  private AbstractAction AALabelAttr;
  private AbstractAction AA1;
  private AbstractAction AA2;
  private String tok, PickTok;


  /**
   * Creates a MainWindow with all its components, initializes Secondo-Server, loads the
   * standard-categories, etc.
   * @param   boolean inAnApplet optional for creating out of an applet 
   * @see <a href="MainWindowsrc.html#MainWindow">Source</a> 
   */
  public HoeseViewer() {
    try {
      UIManager.setLookAndFeel(UIManager.getCrossPlatformLookAndFeelClassName());
    } catch (Exception exc) {
      System.err.println("Error loading L&F: " + exc);
    }
    init(); 
    Cats = new Vector(30, 10);
    context = new ContextPanel(this);
    Cats.add(Category.getDefaultCat());
    //Load Standard Categories
    String Catfile = configuration.getProperty("StandardCat");
    if (Catfile != null) {
      ListExpr le = new ListExpr();
      le.readFromFile(Catfile);
      readAllCats(le);
    }
    initComponents();

    MouseKoordLabel = new JLabel("-------/-------");
    MouseKoordLabel.setFont(new Font("Monospaced", Font.PLAIN, 12));
    MouseKoordLabel.setForeground(Color.black);
    actTimeLabel = new JLabel("No Time");
    actTimeLabel.setFont(new Font("Monospaced", Font.PLAIN, 12));
    actTimeLabel.setForeground(Color.black);
    jMenuBar1.add(Box.createHorizontalGlue());
    jMenuBar1.add(actTimeLabel);
    jMenuBar1.add(Box.createHorizontalStrut(30));
    jMenuBar1.add(MouseKoordLabel);
    JToolBar jtb = new JToolBar();
    jtb.putClientProperty("JToolBar.isRollover", Boolean.TRUE);
    jtb.setFloatable(false);
    jtb.addSeparator();
    //jtb.add(MouseKoordLabel);
    JPanel AnimPanel = new JPanel(new FlowLayout(2, 0, 4));
    AnimPanel.setPreferredSize(new Dimension(60, 12));
    JButton ctrls[] = new JButton[6];
    ctrls[0] = new JButton(new ImageIcon(configuration.getProperty("PlayIcon")));
    ctrls[1] = new JButton(new ImageIcon(configuration.getProperty("ReverseIcon")));
    ctrls[2] = new JButton(new ImageIcon(configuration.getProperty("PlayDefIcon")));
    ctrls[3] = new JButton(new ImageIcon(configuration.getProperty("ToendIcon")));
    ctrls[4] = new JButton(new ImageIcon(configuration.getProperty("TostartIcon")));
    ctrls[5] = new JButton(new ImageIcon(configuration.getProperty("StopIcon")));
    ActionListener al = new AnimCtrlListener();
    for (int i = 0; i < ctrls.length; i++) {
      ctrls[i].setActionCommand(Integer.toString(i));
      ctrls[i].addActionListener(al);
      //ctrls[i].setMinimumSize(new Dimension(8,8));
      ctrls[i].setMargin(new Insets(0, 0, 0, 0));
      jtb.add(ctrls[i]);
    }
    TimeSlider = new JScrollBar(JScrollBar.HORIZONTAL, 0, 0, 0, 0);
    //JSlider(0,300,0);
    TimeSlider.addAdjustmentListener(new TimeAdjustmentListener());
    TimeSlider.setPreferredSize(new Dimension(400, 15));
    TimeSlider.setUnitIncrement(1);
    TimeSlider.setBlockIncrement(60);
    jtb.add(TimeSlider);
    TextDisplay = new TextWindow(this);
    DoQuerySelection = new QueryListSelectionListener();
    allProjection = new AffineTransform();
    allProjection.scale(ZoomFactor, ZoomFactor);
    GraphDisplay = new GraphWindow(this);
    GraphDisplay.addMouseMotionListener(new MouseMotionAdapter() {

      /**
       * 
       * @param e
       * @see <a href="MainWindowsrc.html#mouseMoved">Source</a> 
   */
      public void mouseMoved (MouseEvent e) {
        //Koordinaten in Weltkoordinaten umwandeln
        Point2D.Double p = new Point2D.Double();
        try {
          p = (Point2D.Double)allProjection.inverseTransform(e.getPoint(), 
              p);
        } catch (Exception ex) {}
        MouseKoordLabel.setText(Double.toString(p.getX()).concat("       ").substring(0, 
            7) + "/" + Double.toString(p.getY()).concat("       ").substring(0, 
            7));
      }
    });
    SelectionControl = new SelMouseAdapter();
    GraphDisplay.addMouseListener(SelectionControl);
    SpatioTempPanel = new JPanel(new BorderLayout());
    LayerSwitchBar = new JPanel();
    LayerSwitchBar.setPreferredSize(new Dimension(10, 10));
    LayerSwitchBar.setLayout(new BoxLayout(LayerSwitchBar, BoxLayout.Y_AXIS));
    GraphDisplay.createBackLayer(null, 0, 0);
    GeoScrollPane = new JScrollPane(GraphDisplay);
    SpatioTempPanel.add(GeoScrollPane, BorderLayout.CENTER);
    SpatioTempPanel.add(LayerSwitchBar, BorderLayout.WEST);
    VisComPanel = new JSplitPane(JSplitPane.VERTICAL_SPLIT, SpatioTempPanel,dummy);
    VisComPanel.setOneTouchExpandable(true);
    VisComPanel.setResizeWeight(1);
    TimeDisplay = new TimePanel(this);
    VisualPanel = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, TextDisplay,VisComPanel);
    VisualPanel.setOneTouchExpandable(true);
    VisualPanel.setPreferredSize(new Dimension(800, 600));
    VisualPanel.setResizeWeight(0);
    JPanel pane = new JPanel();
    pane.setLayout(new BorderLayout());
    pane.add(jtb, BorderLayout.NORTH);
    pane.add(VisualPanel, BorderLayout.CENTER);
    pane.setPreferredSize(new Dimension(800, 600));
    setLayout(new BorderLayout());
    add(pane,BorderLayout.CENTER);
    setDivider();
  }

  /**
   * Sets the divider between split-components
   * @see <a href="MainWindowsrc.html#setdivider>Source</a> 
   */
  public void setDivider () {
    VisComPanel.setDividerLocation(0.75);
    VisualPanel.setDividerLocation(200);
  }

  /**
   * 
   * @return The zoomfactor of the GraphWindow 
   * @see <a href="MainWindowsrc.html#getZoomFactor">Source</a> 
   */
  public double getZoomFactor () {
    return  ZoomFactor;
  }

  /**
   * Sets the ZoomFactor to zf
   * @param zf
   * @see <a href="MainWindowsrc.html#setZoomFactor">Source</a> 
   */
  public void setZoomFactor (double zf) {
    ZoomFactor = zf;
  }

  /**
   * Reads all categories out of the Listexpr le
   * @param le 
   * @see <a href="MainWindowsrc.html#readAllCats">Source</a> 
   */
  public void readAllCats (ListExpr le) {
    if (le.first().atomType() != ListExpr.SYMBOL_ATOM)
      return;
    if (!le.first().symbolValue().equals("Categories"))
      return;
    le = le.second();
    while (!le.isEmpty()) {
      Category cat = Category.ConvertLEtoCat(le.first());
      if (cat != null)
        Cats.add(cat);
      le = le.rest();
    }
  }

  /**
   * 
   * @return A listExpr of all the categories in Cats
   * @see <a href="MainWindowsrc.html#writeAllCats">Source</a> 
   */
  public ListExpr writeAllCats () {
    ListExpr le = ListExpr.theEmptyList();
    ListExpr left = le;
    for (int i = 0; i < Cats.size(); i++)
      if (le.isEmpty()) {
        left = ListExpr.cons(Category.ConvertCattoLE((Category)Cats.elementAt(i)), 
            le);
        le = left;
      } 
      else 
        left = ListExpr.append(left, Category.ConvertCattoLE((Category)Cats.elementAt(i)));
    return  ListExpr.twoElemList(ListExpr.symbolAtom("Categories"), le);
  }



  public Frame getMainFrame(){
    if (VC!=null) 
       return VC.getMainFrame();
    else
       return null;
  }



  /**
   * Init. the menu entries.
   * @see <a href="MainWindowsrc.html#initComponents">Source</a> 
   */
  private void initComponents () {              //GEN-BEGIN:initComponents
    jMenuBar1 = new javax.swing.JMenuBar();
   /** file-menu */
    jMenu1 = new javax.swing.JMenu();
    jMenu_NewSession = new javax.swing.JMenuItem();
    jMenu_OpenSession = new javax.swing.JMenuItem();
    jMenu_SaveSession = new javax.swing.JMenuItem();
    jSeparator1 = new javax.swing.JSeparator();
    jMenu_Browse = new javax.swing.JMenuItem();
    jMenu_Import = new javax.swing.JMenuItem();
    jMenu_Export = new javax.swing.JMenuItem();
    jSeparator2 = new javax.swing.JSeparator();
    jMenu_Exit = new javax.swing.JMenuItem();


 /** Menu gui */
    jMenuGui = new javax.swing.JMenu();
    MINewCat = new javax.swing.JMenuItem();
    delQueryMenu = new javax.swing.JMenuItem();
    isAutoCatMI = new javax.swing.JCheckBoxMenuItem();
    jSeparator5 = new javax.swing.JSeparator();
    MIsetKontext = new javax.swing.JCheckBoxMenuItem();
    MILayerMgmt = new javax.swing.JMenuItem();
    MenuObject = new javax.swing.JMenu();
    MIHideObject = new javax.swing.JMenuItem();
    MIShowObject = new javax.swing.JMenuItem();
    jMenuItem8 = new javax.swing.JMenuItem();
    MIMoveTop = new javax.swing.JMenuItem();
    MIMoveUp = new javax.swing.JMenuItem();
    MIMoveDown = new javax.swing.JMenuItem();
    MIMoveBottom = new javax.swing.JMenuItem();
    jMenuBar1.setFont(new java.awt.Font("Dialog", 0, 10));
    RBMICustTI = new JRadioButtonMenuItem();


    /** File -Menu **/
    jMenu1.setText("File");
    jMenu_NewSession.setText("new Session");
    jMenu_NewSession.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
         on_jMenu_NewSession(evt);
      }
    });
    jMenu1.add(jMenu_NewSession);
    jMenu_OpenSession.setText("open Session");
    jMenu_OpenSession.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        on_jMenu_OpenSession(evt);
      }
    });
    jMenu1.add(jMenu_OpenSession);

    jMenu_SaveSession.setText("save Session");
    jMenu_SaveSession.addActionListener(new java.awt.event.ActionListener() {
       public void actionPerformed (java.awt.event.ActionEvent evt) {
          on_jMenu_SaveSession(evt);
       }
    });
    jMenu1.add(jMenu_SaveSession);
    jMenu1.add(jSeparator1);

    JMenuItem MIloadCat = new JMenuItem("load Category");
    MIloadCat.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        final JFileChooser fc = new JFileChooser(configuration.getProperty("WorkingDir","/"));
        int returnVal = fc.showOpenDialog(HoeseViewer.this);
        if (returnVal == JFileChooser.APPROVE_OPTION) {
          File file = fc.getSelectedFile();
          ListExpr le = new ListExpr();
          //CommandDisplay.appendText("Adding categories from " + file.getPath()+ "...");
          String suc;
          if (le.readFromFile(file.getPath()) == 0) {
            Cats = new Vector(30, 10);
            Cats.add(Category.getDefaultCat());
            suc = "OK";
            readAllCats(le);
          } 
          else 
            suc = "Failed";
        }
      }
    });

    jMenu1.add(MIloadCat);

    JMenuItem MISaveCat = new JMenuItem("save Category");
    MISaveCat.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        final JFileChooser fc = new JFileChooser(configuration.getProperty("WorkingDir","/"));
        int returnVal = fc.showSaveDialog(HoeseViewer.this);
        if (returnVal == JFileChooser.APPROVE_OPTION) {
          File file = fc.getSelectedFile();
          ListExpr le = writeAllCats();
          //CommandDisplay.appendText("Writing Categories to " + file.getPath()+ "...");
          String suc;
          suc = (le.writeToFile(file.getPath()) == 0) ? "OK" : "Failed";
          //CommandDisplay.appendText(suc + "\n");
          //CommandDisplay.showPrompt();
        }
      }
    });

    jMenu1.add(MISaveCat);

    MenuExtension.addMenu(jMenu1); 
    jMenuBar1.add(jMenu1);


    jMenuGui.setText("gui");
    isAutoCatMI.setText("auto Category");
    jMenuGui.add(isAutoCatMI);

    MIQueryRep = new JMenuItem("query representation");
    MIQueryRep.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        QueryResult qr = (QueryResult)TextDisplay.getQueryCombo().getSelectedItem();
        if (qr!=null){
           GraphDisplay.newQueryRepresentation(qr.getGraphObjects());
           GraphDisplay.repaint();
        }
      }
    });

    jMenuGui.add(MIQueryRep);
    jMenuGui.add(jSeparator5);
    AACatEdit = new AbstractAction("CategoryEditor"){ 
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        new CategoryEditor(HoeseViewer.this, true).show();
        GraphDisplay.repaint();
      }
    };

    jMIShowCE = jMenuGui.add(AACatEdit);

    AAContext = new AbstractAction("set Context") {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        MIsetKontext.setSelected(!MIsetKontext.isSelected());
        on_Set_Kontext();
      }
    };

    MIsetKontext = jMenuGui.add(AAContext);

    AAZoomOut = new AbstractAction("Zoom Out"){ 
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        double zf = 1/ZoomFactor;
        ZoomFactor = 1;
        ClipRect = null;
        Point p = GeoScrollPane.getViewport().getViewPosition();
        updateViewParameter();
        GeoScrollPane.getViewport().setViewPosition(new Point((int)(p.getX()*zf), 
            (int)(p.getY()*zf)));
      }
    };
    MIZoomOut = jMenuGui.add(AAZoomOut);

    MIZoomMinus = new JMenuItem("Zoom -");
    MIZoomMinus.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        double zf = 0.75;
        if (ZoomFactor*zf < 1.3) {
          zf = 1/ZoomFactor;
          ZoomFactor = 1;
          ClipRect = null;
          Point p = GeoScrollPane.getViewport().getViewPosition();
          updateViewParameter();
          GeoScrollPane.getViewport().setViewPosition(new Point((int)(p.getX()*zf), 
              (int)(p.getY()*zf)));
          return;
        }
        ZoomFactor *= zf;
        double m[] = new double[6];
        allProjection.getMatrix(m);
        m[0] *= zf;
        m[3] *= zf;
        m[4] *= zf;
        m[5] *= zf;
        allProjection = new AffineTransform(m);
        BBoxDC.setSize((int)(BBoxDC.getWidth()*zf), (int)(BBoxDC.getHeight()*zf));
        GraphDisplay.updateLayersSize(BBoxDC);
        if (!context.AutoProjRB.isSelected()) {
          if (hasBackImage)
            LayerSwitchBar.remove(0);
          if (context.ImagePath != null)
            addSwitch(GraphDisplay.createBackLayer(context.ImagePath, context.getMapOfs().getX()*zf, 
                context.getMapOfs().getY()*zf), 0); 
          else 
            GraphDisplay.createBackLayer(null, 0, 0);
        }
        Point p = GeoScrollPane.getViewport().getViewPosition();
        GeoScrollPane.getViewport().setViewPosition(new Point((int)(p.getX()*zf), 
            (int)(p.getY()*zf)));
        GraphDisplay.repaint();
      }
    });
    jMenuGui.add(MIZoomMinus);

    MILayerMgmt.setText("LayerManagement");
    jMenuGui.add(MILayerMgmt);
    MILayerMgmt.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        new LayerMgmt(HoeseViewer.this, GraphDisplay.getComponents()).show();
        GraphDisplay.repaint();
      }
    });

    jMenuGui.add(new JSeparator());
    String TextPar[] =  {
      "m/h", "h/d", "d/w", "w/M", "M/Y"
    };
    JRadioButtonMenuItem RBMITimeFrame[] = new JRadioButtonMenuItem[TextPar.length];
    ButtonGroup bg = new ButtonGroup();
    final int SliderPar[] =  {
      1, 60, 1440, 7*1440, 30*1440, (int)(365.25*1440)
    };
    ActionListener TimeFrameListener = new ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        int index = Integer.parseInt(evt.getActionCommand());
        TimeSlider.setUnitIncrement(SliderPar[index]);
        TimeSlider.setBlockIncrement(SliderPar[index + 1]);
      }
    };
    tok = "time";
    for (int i = 0; i < RBMITimeFrame.length; i++) {
      RBMITimeFrame[i] = new JRadioButtonMenuItem(tok + " " + TextPar[i]);
      RBMITimeFrame[i].setActionCommand(Integer.toString(i));
      RBMITimeFrame[i].addActionListener(TimeFrameListener);
      jMenuGui.add(RBMITimeFrame[i]);
      bg.add(RBMITimeFrame[i]);
    }
    RBMITimeFrame[0].setSelected(true);
    RBMICustTI.setText(tok + " =");
    jMenuGui.add(RBMICustTI);
    bg.add(RBMICustTI);
    RBMICustTI.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        String input = JOptionPane.showInputDialog("Insert a time increment in min.");
        int l = 1;
        try {
          l = Integer.parseInt(input);
        } catch (NumberFormatException n) {}
        RBMICustTI.setText(tok + " = " + Integer.toString(l) + " min.");
        TimeSlider.setUnitIncrement(l);
        TimeSlider.setBlockIncrement(l);
      }
    });
    //RBMITimeFrame		
    jMenuBar1.add(jMenuGui);
    MenuExtension.addMenu(jMenuGui);

    MenuObject.setText("Object");
    MIHideObject.setText("hide");
    MIHideObject.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        QueryResult qr = null;
        qr = (QueryResult)TextDisplay.getQueryCombo().getSelectedItem();
        if (qr != null) {
          Object o = qr.getSelectedValue();
          if (o instanceof DsplBase) {
            qr.clearSelection();
            ((DsplBase)o).setVisible(false);
            GraphDisplay.repaint();
          } 
          else 
            statusBeep("No DsplBase object selected!");
        } 
        else 
          statusBeep("No query selected!");
      }
    });

    MenuObject.add(MIHideObject);
    MIShowObject.setText("show");
    MIShowObject.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        QueryResult qr = null;
        qr = (QueryResult)TextDisplay.getQueryCombo().getSelectedItem();
        if (qr != null) {
          Object o = qr.getSelectedValue();
          if (o instanceof DsplBase) {
            ((DsplBase)o).setVisible(true);
            GraphDisplay.repaint();
            TextDisplay.repaint();
          } 
          else 
            statusBeep("No DsplBase object selected!");
        } 
        else 
          statusBeep("No query selected!");
      }
    });
    MenuObject.add(MIShowObject);

    AAViewCat = new AbstractAction("change Category"){
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        if (selGraphObj == null) {
          statusBeep("No DsplGraph object selected!");
          return;
        }
        CategoryEditor ce = new CategoryEditor(HoeseViewer.this, true, selGraphObj.getCategory());
        ce.show();
        if (ce.getActualCategory() != null) {
          selGraphObj.setCategory(ce.getActualCategory());
          GraphDisplay.repaint();
        }
      }
    };
    MINewCat = MenuObject.add(AAViewCat);
    AALabelAttr = new AbstractAction("Label Attributes"){

      /**
       * 
       * @param evt
       * @see <a href="MainWindowsrc.html#MainWindow">Source</a> 
   */
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        if (selGraphObj == null) {
          statusBeep("No DsplGraph object selected!");
          return;
        }
        new LabelAttrDlg(HoeseViewer.this, selGraphObj).show();
        GraphDisplay.repaint();
      }
    };
    MILabelAttr = MenuObject.add(AALabelAttr);
    MenuObject.add(new JSeparator());
    MIMoveTop.setText("Move To Top");
    MIMoveTop.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        if (selGraphObj == null) {
          statusBeep("No DsplGraph object selected!");
          return;
        }
        //oldLayer.getGeoObjects().remove(selGraphObj);
        int newno = GraphDisplay.highestLayer();
        Component[] Comps = GraphDisplay.getComponentsInLayer(newno);
        Layer oldLayer = selGraphObj.getLayer();
        oldLayer.setSelectedButton(false);
        oldLayer.removeGO(selGraphObj);
        Layer newLayer = (Layer)Comps[0];
        //selGraphObj.getSelected = false;
        selGraphObj.setLayer(newLayer);
        newLayer.addGO(-1, selGraphObj);
        newLayer.setSelectedButton(true);
        GraphDisplay.repaint();
      }
    });
    MenuObject.add(MIMoveTop);

    MIMoveUp.setText("Move Layer Up");
    MIMoveUp.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        if (selGraphObj == null) {
          statusBeep("No DsplGraph object selected!");
          return;
        }
        //oldLayer.getGeoObjects().remove(selGraphObj);
        Layer oldLayer = selGraphObj.getLayer();
        int aktno = GraphDisplay.getIndexOf(oldLayer);
        if (aktno > 0)
          aktno--;
        // Component [] Comps =
        //         GraphDisplay.getComponentsInLayer(
        //         aktno);
        oldLayer.setSelectedButton(false);
        oldLayer.removeGO(selGraphObj);
        Layer newLayer = (Layer)GraphDisplay.getComponent(aktno);
        //selGraphObj.getSelected = false;
        selGraphObj.setLayer(newLayer);
        newLayer.addGO(-1, selGraphObj);
        newLayer.setSelectedButton(true);
        GraphDisplay.repaint();
      }
    });
    MenuObject.add(MIMoveUp);
    MIMoveDown.setText("Move layer Down");
    MIMoveDown.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        if (selGraphObj == null) {
          statusBeep("No DsplGraph object selected!");
          return;
        }
        //oldLayer.getGeoObjects().remove(selGraphObj);
        int min = hasBackImage ? 2 : 1;
        Layer oldLayer = selGraphObj.getLayer();
        int aktno = GraphDisplay.getIndexOf(oldLayer);
        if (aktno < GraphDisplay.getComponentCount() - min)
          aktno++;
        // Component [] Comps =
        //         GraphDisplay.getComponentsInLayer(
        //         aktno);
        oldLayer.setSelectedButton(false);
        oldLayer.removeGO(selGraphObj);
        Layer newLayer = (Layer)GraphDisplay.getComponent(aktno);
        //selGraphObj.getSelected = false;
        selGraphObj.setLayer(newLayer);
        newLayer.addGO(0, selGraphObj);
        newLayer.setSelectedButton(true);
        GraphDisplay.repaint();
      }
    });
    MenuObject.add(MIMoveDown);

    MIMoveBottom.setText("Move To Bottom");
    MIMoveBottom.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed (java.awt.event.ActionEvent evt) {
        if (selGraphObj == null) {
          statusBeep("No DsplGraph object selected!");
          return;
        }
        int min = hasBackImage ? 2 : 1;
        Layer oldLayer = selGraphObj.getLayer();
        oldLayer.setSelectedButton(false);
        oldLayer.removeGO(selGraphObj);
        Layer newLayer = (Layer)GraphDisplay.getComponent(GraphDisplay.getComponentCount()
            - min);
        //selGraphObj.getSelected = false;
        selGraphObj.setLayer(newLayer);
        newLayer.addGO(0, selGraphObj);
        newLayer.setSelectedButton(true);
        GraphDisplay.repaint();
      }
    });
    MenuObject.add(MIMoveBottom);
    jMenuBar1.add(MenuObject);
    MenuExtension.addMenu(MenuObject);
   // setJMenuBar(jMenuBar1);
  }             //GEN-END:initComponents




/** remove selected query **/
private boolean removeSelectedQuery(){
   QueryResult qr = (QueryResult)TextDisplay.getQueryCombo().getSelectedItem();
   if (qr == null) {
      statusBeep("No query present!");
      return false;
   }
   qr.clearSelection();
   ListIterator li = qr.getGraphObjects().listIterator();
   while (li.hasNext()) {
      DsplGraph dg = (DsplGraph)li.next();
      Layer l = dg.getLayer();
      Vector go = l.getGeoObjects();
      go.remove(dg);
      if (go.isEmpty()) {
          LayerSwitchBar.remove(l.button);
          GraphDisplay.remove(l);
      }
   }
   qr.getGraphObjects().clear();
   updateViewParameter();
   TextDisplay.getQueryCombo().removeItem(qr);
   return true;
}



/** returns the index of qr in TextDisplay-ComboBox, 
  * if qr not in this box then -1 is returned
  **/
private int getQueryIndex(QueryResult qr){
   JComboBox CB = TextDisplay.getQueryCombo();
   int count = CB.getItemCount();
   int pos = -1; 
   for(int i=0;i<count;i++)
      if (qr.equals(CB.getItemAt(i))) pos = i;
   return pos;
}


public MenuVector getMenuVector(){
    return MenuExtension;
}

/** Removes a SecondoObject */
public void removeObject(SecondoObject o){
   QueryResult qr = new QueryResult(o.getName(),o.toListExpr());
   int index = getQueryIndex(qr);
   if(index>=0){
      JComboBox CB = TextDisplay.getQueryCombo();
      CB.setSelectedIndex(index);
      removeSelectedQuery();
   }
}


/** return true if o is displayed false otherwise */
public boolean isDisplayed(SecondoObject o){
   QueryResult q = new QueryResult(o.getName(),o.toListExpr());
   return getQueryIndex(q) >=0;
}


/** select o */
public boolean selectObject(SecondoObject o){
    QueryResult q = new QueryResult(o.getName(),o.toListExpr());
    int index = getQueryIndex(q);
    if(index <0)
       return false;
    else{
      JComboBox CB = TextDisplay.getQueryCombo();
      CB.setSelectedIndex(index);
      return true;
    }
}  

/** all objects can displayed (see work of Th.Hoese) **/
public boolean canDisplay(SecondoObject o){ return true; }



  /**
   * Shows/hides the context-panel where the TextWindowis displayed. Create an extra temporal projection-layer
   * @see <a href="MainWindowsrc.html#on_Set_Kontext">Source</a> 
   */
  public void on_Set_Kontext () {
    if (MIsetKontext.isSelected()) {
      JLabel l = context.getProjectionLabel();
      //if (ClipRect == null)
      //  l.setBounds(BBoxDC); 
      //else 
      l.setBounds(0, 0, GraphDisplay.getWidth(), GraphDisplay.getHeight());                     // oder BBoxDC
      l.setVisible(false);
      GraphDisplay.add(l, new Integer(20000));
      GraphDisplay.removeMouseListener(SelectionControl);
      GraphDisplay.addMouseListener(context.getProjectionControl());
      VisualPanel.setLeftComponent(context);
    } 
    else {
      GraphDisplay.removeMouseListener(context.getProjectionControl());
      GraphDisplay.addMouseListener(SelectionControl);
      GraphDisplay.remove(context.getProjectionLabel());
      VisualPanel.setLeftComponent(TextDisplay);
    }
    // context.setVisible(true);
  }

  /**
   * Adds a new QueryResult qr to the Textwindow
   * @param qr
   * @return True if no error has occured
   * @see <a href="MainWindowsrc.html#addQueryResult">Source</a> 
   */
  public boolean addQueryResult (QueryResult qr) {
    CurrentQueryResult = qr;
    CurrentQueryResult.addListSelectionListener(DoQuerySelection);
    ListExpr displayErrorList = TextDisplay.newQueryResult(CurrentQueryResult);
    int displayErrorCode = displayErrorList.first().intValue();
    // If an error happened when showing the query result, shows the error
    // message.
    if (displayErrorCode != NOT_ERROR_CODE) {
      // CommandDisplay.appendErr("Error:\n");
      // CommandDisplay.showPrompt();
      return  false;
      //this.sysPanel.addErrorList(displayErrorList.second());
    } 
    else {
      // CommandDisplay.appendText("OK\n");
      // CommandDisplay.showPrompt();
      return  true;
      //QueryResultList.add (CurrentQueryResult);
    }
  }


 /** return the name of this Viewer */
 public String getName(){
    return "Th.Hoese-Viewer";
 }
  
 /** adds a new SecondoObject to this Viewer **/
  public boolean addObject(SecondoObject o){
      if (addQueryResult(new QueryResult(o.getName(), o.toListExpr() ))) {
        if (!CurrentQueryResult.getGraphObjects().isEmpty())
          addSwitch(GraphDisplay.addLayerObjects(CurrentQueryResult.getGraphObjects()),-1);
        CurrentQueryResult.setSelectedIndex(0);
      }
      return true;
  } 



  /**
   * 
   * @return The selected grph. object 
   * @see <a href="MainWindowsrc.html#getSelGO">Source</a> 
   */
  public DsplGraph getSelGO () {
    return  selGraphObj;
  }

  /**
   * Shows the Commandpanel instead of Timepanel when it isn't already visible
   * @see <a href="MainWindowsrc.html#showCommandPanel">Source</a> 
   */
  /*public void showCommandPanel () {
    if (!VisComPanel.getBottomComponent().equals(CommandDisplay)) {
      VisComPanel.setBottomComponent(CommandDisplay);
      //VisComPanel.setDividerLocation(oldDL);
    }
  }
  */

  /**
   * Sets actual global timeinterval
   * @param in Interval 
   * @see <a href="MainWindowsrc.html#setActualTime">Source</a> 
   */
  public void setActualTime (Interval in) {
    TimeBounds = in;
    if (in == null) {
      TimeSlider.setValues(0, 1, 0, 0);
      actTimeLabel.setText("no time");
      TimeSlider.setVisible(false);
      ActualTime = 0;
    } 
    else {
      TimeSlider.setVisible(true);
      ActualTime = TimeBounds.getStart();
      TimeSlider.setValues((int)Math.round(in.getStart()*1440), 1, (int)Math.round(in.getStart()*1440), 
          (int)Math.round(in.getEnd()*1440) + 1);
      actTimeLabel.setText(LEUtils.convertTimeToString(ActualTime));
    }
  }

  /**
   * optional
   * @param evt
   * @see <a href="MainWindowsrc.html#on_jMenu_Browse">Source</a> 
   */
  private void on_jMenu_Browse (java.awt.event.ActionEvent evt) {               //GEN-FIRST:event_on_jMenu_Browse
    // Add your handling code here:
  }             //GEN-LAST:event_on_jMenu_Browse

/** Save session to selected file 
 * @see <a href="MainWindowsrc.html#on_jMenu_SaveSession">Source</a> 
   */
  private void on_jMenu_SaveSession (java.awt.event.ActionEvent evt) {          //GEN-FIRST:event_on_jMenu_SaveSession
    // Add your handling code here:
    final JFileChooser fc = new JFileChooser(configuration.getProperty("WorkingDir", 
        "/"));
    fc.setDialogTitle("Save Session");
    int returnVal = fc.showSaveDialog(HoeseViewer.this);
    if (returnVal == JFileChooser.APPROVE_OPTION) {
      File file = fc.getSelectedFile();
      //					File file=new File("Session");
      ListExpr le = ListExpr.fourElemList(ListExpr.symbolAtom("session"), context.getContextLE(), 
          writeAllCats(), TextDisplay.convertAllQueryResults());
      // CommandDisplay.appendText("Writing Session to " + file.getPath() + "...");
      String suc;
      suc = (le.writeToFile(file.getPath()) == 0) ? "OK" : "Failed";
      // CommandDisplay.appendText(suc + "\n");
      // CommandDisplay.showPrompt();
    }
  }             
/** Loads session from a file
 * @see <a href="MainWindowsrc.html#on_jMenu_OpenSession">Source</a> 
   */  
  private void on_jMenu_OpenSession (java.awt.event.ActionEvent evt) {          //GEN-FIRST:event_on_jMenu_OpenSession
    final JFileChooser fc = new JFileChooser(configuration.getProperty("WorkingDir", 
        "/"));
    fc.setDialogTitle("Open Session");
    int returnVal = fc.showOpenDialog(HoeseViewer.this);
    if (returnVal == JFileChooser.APPROVE_OPTION) {
      File file = fc.getSelectedFile();
      ListExpr le = new ListExpr();
      String suc = ((le.readFromFile(file.getPath()) == 0) && (le.listLength()
          == 4) && (le.first().atomType() == ListExpr.SYMBOL_ATOM) && (le.first().symbolValue().equals("session"))) ?
          "OK" : "FAILED";
      if (suc.equals("FAILED"))
        return;
      le = le.rest();
      Cats = new Vector(10, 5);
      Cats.add(Category.getDefaultCat());
      context.setContextLE(le.first());
      readAllCats(le.second());
      TextDisplay.readAllQueryResults(le.third());
      // inform the ViewerControl over the new Objects
      if(VC!=null){
         JComboBox CB = TextDisplay.getQueryCombo();
         int count = CB.getItemCount();
         SecondoObject SO;
         QueryResult QR;
         for(int i=0;i<count;i++){
            QR = (QueryResult)CB.getItemAt(i);
            SO = new SecondoObject();
            SO.setName(QR.getCommand());
            SO.fromList(QR.getListExpr());
            VC.addObject(SO);
         }
      } 

    }
    // Add your handling code here:
  }             //GEN-LAST:event_on_jMenu_OpenSession

  /**
   * Creates new session by constructing a new instance of mainWindow
   * @param evt
   * @return A new MainWindow
   * @see <a href="MainWindowsrc.html#on_jMenu_NewSession">Source</a> 
   */

  private void on_jMenu_NewSession (java.awt.event.ActionEvent evt) {                     //GEN-FIRST:event_on_jMenu_NewSession
   JComboBox CB = TextDisplay.getQueryCombo();
   int count = CB.getItemCount();
   while (count!=0) {
      QueryResult qr = (QueryResult) CB.getItemAt(0);
      qr.clearSelection();
      ListIterator li = qr.getGraphObjects().listIterator();
      while (li.hasNext()) {
         DsplGraph dg = (DsplGraph)li.next();
         Layer l = dg.getLayer();
         Vector go = l.getGeoObjects();
         go.remove(dg);
         if (go.isEmpty()) {
             LayerSwitchBar.remove(l.button);
             GraphDisplay.remove(l);
        }
      }
      qr.getGraphObjects().clear();
      updateViewParameter();
      TextDisplay.getQueryCombo().removeItem(qr);
      CB = TextDisplay.getQueryCombo();
      count = CB.getItemCount();
   }
   // remove the categorys
    Cats = new  Vector(30,10);
    context = new ContextPanel(this);
    Cats.add(Category.getDefaultCat());

    VisComPanel.setBottomComponent(dummy);
    context.applyButtonPressed();
    String Catfile = configuration.getProperty("StandardCat");
    if (Catfile != null) {
      ListExpr le = new ListExpr();
      le.readFromFile(Catfile);
      readAllCats(le);
    }
    on_Set_Kontext(); 

 }             //GEN-LAST:event_on_jMenu_NewSession



  /**
  Adds the scale of the ZoomFactor to the matrix of th e at transform

   * @see <a href="MainWindowsrc.html#addScaling">Source</a> 
   */
  public AffineTransform addScaling (AffineTransform at) {
    //at.scale(ZoomFactor,ZoomFactor);
    double w[] = new double[6];
    at.getMatrix(w);
    double z = ZoomFactor;
    w[0] *= z;
    w[3] *= z;
    w[4] *= z;
    w[5] *= z;
    at = new AffineTransform(w);
    return  at;
  }

  /**
   * Calc. the projection that all objects fit into visible window with border
   * @return Thecalc. transformation
   * @see <a href="MainWindowsrc.html#calcProjection">Source</a> 
   */
  private AffineTransform calcProjection () {
    double extra = context.getBordSpc();        //extra portion at every border of 30 pix
    Rectangle2D.Double BBWC = context.getWBB();
    System.out.println("bbwc:" + BBWC.toString());
    double wp1x = BBWC.getX();
    double wp1y = BBWC.getY();
    double wpw = BBWC.getWidth();
    double wph = BBWC.getHeight();
    double w = ClipRect.getWidth();             //(double) GeoScrollPane.getViewport().getWidth();
    double h = ClipRect.getHeight();            //(double) GeoScrollPane.getViewport().getHeight();
    //ClipRect.setSize((int)w,(int)h);
    System.out.println("ClipRect:" + ClipRect.toString());
    // if no objects or only a single point,line is visible
    if ((wpw == 0) && (wph == 0)) {
      return  new AffineTransform(1, 0, 0, 1, -wp1x + extra, -wp1y + extra);
    } 
    else if (wpw == 0)
      wpw = 1; 
    else if (wph == 0)
      wph = 1;
    // now division by zero impossible	
    double m00, m11;
    if (w/wpw > h/wph) {        //keep aspect ratio
      //h-=60;  
      m11 = (2*extra - h)/wph;
      m00 = -m11;
    } 
    else {
      //w-=60;
      m00 = (w - 2*extra)/wpw;
      m11 = -m00;
    }
    double m02 = extra - m00*wp1x;
    double m12 = extra - (wph + wp1y)*m11;
    double m01 = 0.0;
    double m10 = 0.0;
    return  new AffineTransform(m00, m10, m01, m11, m02, m12);
  }

  /**
   * When new objects are added or old areremoved or the context has change this
   * method must be called to calc. Layersize and transformation again.
   * @see <a href="MainWindowsrc.html#updateViewParameter">Source</a> 
   */
  public void updateViewParameter () {
    //ClipRect=context.getClip();
    boolean isAuto = context.AutoProjRB.isSelected();
    GraphDisplay.updateBoundingBox();           //fuer neue oder geloeschte Objekte
    if (isAuto) {
      if (ClipRect == null) {
        double w = (double)GeoScrollPane.getViewport().getWidth();
        double h = (double)GeoScrollPane.getViewport().getHeight();
        ClipRect = new Rectangle(0, 0, (int)w, (int)h);
      }
      allProjection = calcProjection();         //addScaling(calcProjection());
    } 
    else {
      allProjection.setTransform(addScaling(context.calcManProjection()));
    }
    //if (ClipRect == null)
    Rectangle rDC = (Rectangle)allProjection.createTransformedShape(context.getWBB()).getBounds();
    //		} catch(Exception ex){
    //	System.out.println(ex);
    //	}
    double x = rDC.getX();
    double y = rDC.getY();
    double w = rDC.getWidth();
    double h = rDC.getHeight();
    //if (x>0) 
    w += 2*context.getBordSpc();                //plus extra space
    //if (y>0) 
    h += 2*context.getBordSpc();
    BBoxDC = new Rectangle(0, 0, (int)w, (int)h);
    //System.out.println("BBox:,BBoxWC");
    GraphDisplay.updateLayersSize(BBoxDC);
    if (!isAuto) {
      if (hasBackImage)
        LayerSwitchBar.remove(0);
      if (context.ImagePath != null)
        addSwitch(GraphDisplay.createBackLayer(context.ImagePath, context.getMapOfs().getX(), 
            context.getMapOfs().getY()), 0); 
      else 
        GraphDisplay.createBackLayer(null, 0, 0);
      //mw.LayerSwitchBar.add(mw.GraphDisplay.createBackLayer(ImagePath),0);
      hasBackImage = (context.ImagePath != null);
      //else 
      // GraphDisplay.updateLayersSize(ClipRect);
    }
    GraphDisplay.repaint();
  }

  /**
   * Writes s to the CommandPanel
   * @param s
   * @see <a href="MainWindowsrc.html#writeCommand">Source</a> 
   */
  public static void writeCommand (String s) {
    //frame.CommandDisplay.appendText(s + "\n");
    //frame.CommandDisplay.showPrompt();
  }

  /**
   * This method allows to any class to command to this SecondoJava object to
   * execute a Secondo command, and this object will execute the Secondo command
   * and show the result in its result windows (this is, TextWindow for a query
   * result, CommandPanel for result from another command, GraphWindow for graph.
   * objects) and writes the error
   * messages in the command-panel.
   * 
   * @param command The user command
   * @see <a href="MainWindowsrc.html#execUserCommand">Source</a> 
   */
  public void execUserCommand (String command) {
    System.out.println(command);
    ListExpr displayErrorList;
    int displayErrorCode;
    ListExpr resultList = new ListExpr();
    int commandLevel = 0;
    IntByReference errorCode = new IntByReference(0);
    IntByReference errorPos = new IntByReference(0);
    StringBuffer errorMessage = new StringBuffer();
    // First send an "echo" to the system panel with the received command.
    //CommandDisplay.appendText("\n" + command + "...");
    // Builds the data to send to the server.
    if (command.startsWith("(")) {
      // if command is a list representation, then the command level to use
      // is EXEC_COMMAND_LISTEXPR_SYNTAX.
     // commandLevel = secondoInterface.EXEC_COMMAND_LISTEXPR_SYNTAX;
    } 
    else {
      // if command is not a list representation, then the command level to
      // use is EXEC_COMMAND_SOS_SYNTAX.
      // commandLevel = secondoInterface.EXEC_COMMAND_SOS_SYNTAX;
    }
    // Executes the remote command.
    /*secondoInterface.Secondo(command,           //Command to execute.
    ListExpr.theEmptyList(),                    // we don't use it here.
    commandLevel, true,         // command as text.
    false,      // result as ListExpr.
    resultList, errorCode, errorPos, errorMessage); */
    // If any error had happened, send it to the sysPanel and execUserCommand
    // finish.
    if (errorCode.value != 0) {
      //CommandDisplay.appendText("\n");
      // Sends the system messages to the sysPannel.
      // Shows the error message.
      //CommandDisplay.appendErr("Error in Secondo command:\n");
      //CommandDisplay.appendErr(ServerErrorCodes.getErrorMessageText(errorCode.value)+ "\n");
      //The ErrorPos is not writed because it is not used by Secondo yet.
      if (errorMessage.length() > 0) {          // If the errorMessage is not empty.
        //CommandDisplay.appendErr(errorMessage.toString() + "\n");
      }
      //this.sysPanel.addErrorList(resultList);
      // and shows again the prompt.
      //CommandDisplay.showPrompt();
      return;
    }
    // If the code reach this point, no error was returned by the server, and
    // resultList contains the answer to the command.
    // It shows the result.
    if (command.startsWith("query") || command.startsWith("(query")) {
      // If the command was a query.
      // Prints the query result in the textWindow.
      CurrentQueryResult = new QueryResult(command, resultList);
      CurrentQueryResult.addListSelectionListener(DoQuerySelection);
      displayErrorList = TextDisplay.newQueryResult(CurrentQueryResult);
      displayErrorCode = displayErrorList.first().intValue();
      // If an error happened when showing the query result, shows the error
      // message.
      if (displayErrorCode != NOT_ERROR_CODE) {
        //CommandDisplay.appendText("\n");
        //CommandDisplay.appendErr("Error:\n" + ServerErrorCodes.getErrorMessageText(displayErrorCode));
        //this.sysPanel.addErrorList(displayErrorList.second());
      } 
      else {
        //CommandDisplay.appendText("OK\n");
        //QueryResultList.add (CurrentQueryResult);
        if (!CurrentQueryResult.getGraphObjects().isEmpty())
          addSwitch(GraphDisplay.addLayerObjects(CurrentQueryResult.getGraphObjects()), 
              -1);
        CurrentQueryResult.setSelectedIndex(0);
        //	System.out.println(LayerSwitchBar.isValid());
        // It puts in the top the textWindow where the result is shown.
        //((TextFrame)textWindow.elementAt(0)).toFront();
      }
    } 
    else {
      // If it is not a query.
      if (resultList.isEmpty()) {               //If resultList is empty.
        //CommandDisplay.appendText("OK\n");
      } 
      else {
        //CommandDisplay.appendText("Result:" + resultList.writeListExprToString() + "\n");
      }
    }
    // and shows again the prompt.
    //CommandDisplay.showPrompt();
  }

  /**
   * Writes txt to commandPanel and beep, for user information,errors ...
   * @param txt
   * @see <a href="MainWindowsrc.html#statusBeep">Source</a> 
   */
  public void statusBeep (String txt) {
    //CommandDisplay.appendErr(txt + "\n");
    Toolkit.getDefaultToolkit().beep();
    //CommandDisplay.showPrompt();
  }

  /**
   * The selected object should be visible even when it moves. This method keeps GO visible
   * @see <a href="MainWindowsrc.html#makeSelectionVisible">Source</a> 
   */
  public void makeSelectionVisible () {
    if (selGraphObj == null)
      return;
    if (selGraphObj.getRenderObject(allProjection) == null)
      return;
    Rectangle2D r = selGraphObj.getRenderObject(allProjection).getBounds2D();
    if (r == null)
      return;                   //movingXXX may be undefined
    //try{
    Shape s = allProjection.createTransformedShape(r);
    //System.out.println(s.getBounds());
    r = s.getBounds2D();
    if (!isMouseSelected) {
      double w = (double)GeoScrollPane.getViewport().getWidth();
      double h = (double)GeoScrollPane.getViewport().getHeight();
      GeoScrollPane.getHorizontalScrollBar().setValue((int)(r.getX() + r.getWidth()/2
          - w/2));
      GeoScrollPane.getVerticalScrollBar().setValue((int)(r.getY() + r.getHeight()/2
          - h/2));
    }
    isMouseSelected = false;
    // selGraphObj.getLayer().scrollRectToVisible(
    //         irect.createIntersection(s.getBounds()).getBounds());
    GraphDisplay.repaint();
  }

  /**
   * Adds a layer-switch to LayerSwitchbar
   * @param tb The button
   * @param index The position
   * @see <a href="MainWindowsrc.html#addSwitch">Source</a> 
   */
  public void addSwitch (JToggleButton tb, int index) {
    if (index < 0)
      LayerSwitchBar.add(tb); 
    else 
      LayerSwitchBar.add(tb, index);
    LayerSwitchBar.revalidate();
    LayerSwitchBar.repaint();
  }


/** Listens to a selection change in a query list
  * @see <a href="MainWindowsrc.html#QueryListSelectionListener">Source</a> 
   */

  class QueryListSelectionListener
      implements ListSelectionListener {

    public void valueChanged (ListSelectionEvent e) {
      Object o;
      if (e.getValueIsAdjusting())
        return;
      QueryResult theList = (QueryResult)e.getSource();
      o = theList.getSelectedValue();
      if (selBaseObj != null) {
        selBaseObj.setSelected(false);
        if (selBaseObj.getFrame() != null) {
          selBaseObj.getFrame().select(null);
          //selBaseObj.getFrame().show(false);
        }
        selBaseObj = null;
      }
      if (selGraphObj != null) {
        selGraphObj.getLayer().setSelectedButton(false);
        selGraphObj.setSelected(false);
        //System.out.println("selection off:"+selGraphObj.getAttrName());
        selGraphObj = null;
        //selGraphObj.getLayer().repaint();
      }
      if (o instanceof Timed) {
        TimeDisplay.setTimeObject((Timed)o);
        oldDL = VisComPanel.getLastDividerLocation();
        VisComPanel.setBottomComponent(TimeDisplay);
        VisComPanel.setDividerLocation(0.8);
      } 
      else 
        //showCommandPanel()
       ;
      if (o instanceof DsplGraph) {
        DsplGraph dgorig = (DsplGraph)o;
        dgorig.setSelected(true);
        dgorig.getLayer().setSelectedButton(true);
        selGraphObj = dgorig;
        if (!isMouseSelected && (selGraphObj instanceof Timed))
          TimeSlider.setValue((int)Math.round(((Timed)selGraphObj).getTimeBounds().getStart()*1440));
        makeSelectionVisible();
      } 
      else if (o instanceof DsplBase) {
        selBaseObj = (DsplBase)o;
        selBaseObj.setSelected(true);
        if (selBaseObj.getFrame() != null) {
          selBaseObj.getFrame().select(o);
          //selBaseObj.getFrame().show(true);
        }
      } 
      else 
        GraphDisplay.repaint();
    }
  
  }
/** Manages mouseclicks in the GraphWindow. It is placed here for textual-interaction 

   * @see <a href="MainWindowsrc.html#SelMouseAdapter">Source</a> 
   */ 
  class SelMouseAdapter extends MouseAdapter
      implements MouseMotionListener {
    JLabel selLabel = null;

    public void mouseReleased (MouseEvent e) {
      if ((e.getModifiers() & InputEvent.BUTTON3_MASK) == InputEvent.BUTTON3_MASK) {
        if (selLabel != null) {
          GraphDisplay.remove(selLabel);
          GraphDisplay.repaint();
          Rectangle2D r = selLabel.getBounds().getBounds2D();
          selLabel = null;
          if ((r.getHeight() < 1) || (r.getWidth() < 1))
            return;
          double w = (double)GeoScrollPane.getViewport().getWidth();
          double h = (double)GeoScrollPane.getViewport().getHeight();
          double zf = Math.min(w/r.getWidth(), h/r.getHeight());
          ZoomFactor *= zf;
          double m[] = new double[6];
          allProjection.getMatrix(m);
          //double z=ZoomFactor;
          m[0] *= zf;
          m[3] *= zf;
          m[4] *= zf;
          m[5] *= zf;
          allProjection = new AffineTransform(m);
          BBoxDC.setSize((int)(BBoxDC.getWidth()*zf), (int)(BBoxDC.getHeight()*zf));
          //System.out.println("ClipRect:"+ClipRect.toString());
          GraphDisplay.updateLayersSize(BBoxDC);
          if (!context.AutoProjRB.isSelected()) {
            if (hasBackImage)
              LayerSwitchBar.remove(0);
            if (context.ImagePath != null)
              addSwitch(GraphDisplay.createBackLayer(context.ImagePath, context.getMapOfs().getX()*zf, 
                  context.getMapOfs().getY()*zf), 0); 
            else 
              GraphDisplay.createBackLayer(null, 0, 0);
          }
          GraphDisplay.scrollRectToVisible(new Rectangle((int)(r.getX()*zf), 
              (int)(r.getY()*zf), (int)w, (int)h));
          //GeoScrollPane.getViewport().setViewPosition(new Point((int)(r.getX()*zf),(int)(r.getY()*zf)));
          GraphDisplay.repaint();
        }
      }
    }

    public void mousePressed (MouseEvent e) {
      //Koordinaten in Weltkoordinaten umwandeln
      if ((e.getModifiers() & InputEvent.BUTTON3_MASK) == InputEvent.BUTTON3_MASK) {
        Rectangle r = new Rectangle(e.getX(), e.getY(), 0, 0);
        selLabel = new JLabel();
        selLabel.setBounds(r);
        selLabel.setBorder(new LineBorder(Color.black, 1));
        //selLabel.setOpaque(true);
        //selLabel.setBackground(new Color(230,30,0,50));
        GraphDisplay.add(selLabel, new Integer(20000));
        //GraphDisplay.removeMouseListener(SelectionControl);
        GraphDisplay.addMouseMotionListener(this);
      }
    }
    public void mouseDragged (MouseEvent e) {
      if ((e.getModifiers() & InputEvent.BUTTON3_MASK) == InputEvent.BUTTON3_MASK) {
        Point p = selLabel.getLocation();
        int x = (int)p.getX();
        int y = (int)p.getY();
        int w = e.getX() - x;
        int h = e.getY() - y;
        if (w < 0) {
          w = -w;               //+selLabel.getWidth();
          x = x - w + 1;
        }
        if (h < 0) {
          h = -h;               //+selLabel.getHeight();
          y = y - h + 1;
        }
        selLabel.setBounds(x, y, w, h);
      }
    }

    public void mouseMoved (MouseEvent e) {}

    public void mouseClicked (MouseEvent e) {
      Point2D.Double p = new Point2D.Double();
      try {
        p = (Point2D.Double)allProjection.inverseTransform(e.getPoint(), p);
      } catch (Exception ex) {}
      //int hits = 0;
      double SelIndex = 10000, BestIndex = -1, TopIndex = -1;
      //boolean selectionfound = false;
      DsplGraph Obj2sel = null, top = null;
      int ComboIndex = -1, TopComboIndex = -1;
      double scalex = 1/Math.abs(allProjection.getScaleX());
      double scaley = 1/Math.abs(allProjection.getScaleY());
      //ListIterator li=QueryResultList.listIterator();
      if ((selGraphObj != null) && (selGraphObj.contains(p.getX(), p.getY(), 
          scalex, scaley)))
        SelIndex = selGraphObj.getLayer().getObjIndex(selGraphObj);
      JComboBox cb = TextDisplay.getQueryCombo();
      for (int j = 0; j < cb.getItemCount(); j++) {
        ListIterator li2 = ((QueryResult)cb.getItemAt(j)).getGraphObjects().listIterator();
        while (li2.hasNext()) {
          DsplGraph dg = (DsplGraph)li2.next();
          if (!dg.getVisible())
            continue;
          if (dg.contains(p.getX(), p.getY(), scalex, scaley)) {
            double AktIndex = (dg.getSelected()) ? SelIndex : dg.getLayer().getObjIndex(dg);
            //System.out.println("A:"+AktIndex);
            //top is the topmost GO
            if (AktIndex > TopIndex) {
              TopIndex = AktIndex;
              top = dg;
              TopComboIndex = j;
            }
            if ((selGraphObj != null) && (AktIndex < SelIndex) && (AktIndex > BestIndex)) {
              //the next GO smaller than selindex and greater than best until now
              Obj2sel = dg;
              BestIndex = AktIndex;
              //System.out.println("B:"+BestIndex+"S: "+SelIndex);
              ComboIndex = j;
            }
          }
        }
      }
      if (Obj2sel == null) {
        Obj2sel = top;
        ComboIndex = TopComboIndex;
      }
      QueryResult qr;
      if (Obj2sel == null) {
        qr = (QueryResult)cb.getSelectedItem();
        if (qr != null)
          qr.clearSelection();
      } 
      else {
        if (Obj2sel.getSelected())
          return;
        isMouseSelected = true;
        cb.setSelectedIndex(ComboIndex);
        qr = (QueryResult)cb.getSelectedItem();
        //qr.clearSelection();
        qr.setSelectedValue(Obj2sel, true);
      }
    }
  }
/** This class controls movement of the timeslider

   * @see <a href="MainWindowsrc.html#TimeAdjustmentListener">Source</a> 
   */
  class TimeAdjustmentListener
      implements AdjustmentListener {

    public void adjustmentValueChanged (AdjustmentEvent e) {
      if (TimeBounds == null) {
        TimeSlider.setValue(0);
        return;
      }
      int v = e.getValue();
      double anf;
      if (v == TimeSlider.getMinimum())
        anf = TimeBounds.getStart(); 
      else {
        anf = (double)v/1440.0;
        if (anf > TimeBounds.getEnd())
          anf = TimeBounds.getEnd();
      }
      //System.out.println("anf"+e.getValue());
      if (anf == ActualTime)
        return;
      ActualTime = anf;
      actTimeLabel.setText(LEUtils.convertTimeToString(ActualTime));
      makeSelectionVisible();
      GraphDisplay.repaint();
      //System.out.println(ActualTime);
    }
  }



  private void init() {
    InputStreamReader configReader = null;
    InputStream configStream = null;
    URL configURL = null;
    String configFileName = null;
    if (inAnApplet) {
    } 
    else {
      // If executed as standalone application.
      configFileName = "file:" + System.getProperty("user.dir") + "/" + this.CONFIGURATION_FILE;
    }
    // Once the address of the configuration file is known, it tries to
    try {
      // Stores the configuration information.
      this.configuration = new Properties();
      configStream = (new URL(configFileName)).openStream();
      this.configuration.load(configStream);
      // Initializes the ~secondoInterface~ object that will be used for
      // any connection with the Secondo server.
      configURL = new URL(configFileName);
      configReader = new InputStreamReader(configURL.openStream());
    } catch (FileNotFoundException except) {
      // If an error happened when trying to open the file.
      if (DEBUG_MODE) {         //Only if debug mode.
        System.err.println("DEBUG MODE: configuration file '" + CONFIGURATION_FILE
            + "` not found when executing in SecondoJava() class.");
        except.printStackTrace();
      }
      System.err.println("Error: configuration file '" + CONFIGURATION_FILE
          + "` not found.");
    } catch (SecurityException except) {
      // If it throws a security exception.
      if (DEBUG_MODE) {         //Only if debug mode.
        System.err.println("DEBUG MODE: Security exception when reading the configuration file '"
            + CONFIGURATION_FILE + "` in SecondoJava() class.");
        except.printStackTrace();
      }
      System.err.println("Security Error: security exception when reading the configuration file '"
          + CONFIGURATION_FILE + "`.");
    } catch (IOException except) {
      // If an error happened when reading the file.
      if (DEBUG_MODE) {         //Only if debug mode.
        System.err.println("DEBUG MODE: configuration file '" + CONFIGURATION_FILE
            + "` not found when executing in SecondoJava() class.");
        except.printStackTrace();
      }
      System.err.println("Error: configuration file '" + CONFIGURATION_FILE
          + "` not found.");
    } finally {
      try {
        configStream.close();
        configReader.close();
      } catch (IOException except) {
      // Does not matter.
      }
    }
  }


  
/** This class manages animation-events * @see <a href="MainWindowsrc.html#MainWindow">Source</a> 
   * @see <a href="MainWindowsrc.html#AnimCtrlListener">Source</a> 
   */ 
  class AnimCtrlListener
      implements ActionListener {
    int inc = 1;
    int dir = 1;
    boolean onlyDefined;
    Vector TimeObjects;
    Timer AnimTimer = new Timer(50, new ActionListener() {

      public void actionPerformed (java.awt.event.ActionEvent evt) {
        int v = TimeSlider.getValue();
        if (onlyDefined) {
          v++;
          int min = Integer.MAX_VALUE;
          ListIterator li = TimeObjects.listIterator();
          while (li.hasNext()) {
            Timed t = (Timed)li.next();
            min = Math.min(Interval.getMinGT(t.getIntervals(), (double)v/1440.0), 
                min);
          }
          if (min < Integer.MAX_VALUE) {
            TimeSlider.setValue(min);
            AnimTimer.setDelay((1000 - TimeObjects.size() < 50) ? 50 : 1000
                - TimeObjects.size());
          } 
          else 
            AnimTimer.stop();
        } 
        else {
          inc = dir*TimeSlider.getUnitIncrement();
          TimeSlider.setValue(v + inc);
          if ((v + inc) != TimeSlider.getValue()) {
            AnimTimer.stop();
          }
        }
      }
    });

    public void actionPerformed (java.awt.event.ActionEvent evt) {
      switch (Integer.parseInt(evt.getActionCommand())) {
        case 0:                 //play
          onlyDefined = false;
          dir = 1;
          AnimTimer.start();
          break;
        case 1:                 //reverse
          onlyDefined = false;
          dir = -1;
          AnimTimer.start();
          break;
        case 2:                 //show at defined Times
          TimeObjects = new Vector(50, 10);
          JComboBox cb = TextDisplay.getQueryCombo();
          for (int j = 0; j < cb.getItemCount(); j++) {
            ListIterator li2 = ((QueryResult)cb.getItemAt(j)).getGraphObjects().listIterator();
            while (li2.hasNext()) {
              Object o = li2.next();
              if ((o instanceof DsplGraph) && (o instanceof Timed))
                TimeObjects.add(o);
            }
          }
          onlyDefined = true;
          dir = 1;
          AnimTimer.start();
          break;
        case 3:                 //to end
          TimeSlider.setValue(TimeSlider.getMaximum());
          break;
        case 4:                 //to start
          TimeSlider.setValue(TimeSlider.getMinimum());
          break;
        case 5:                 //stop
          AnimTimer.stop();
          break;
          //StartButton.setSelected(!StartButton.isSelected());	
      }
  }
    }
}




