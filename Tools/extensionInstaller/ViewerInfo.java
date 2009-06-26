import java.util.*;
import org.w3c.dom.*;
import javax.xml.parsers.*;
import java.io.*;
import java.util.zip.*;
import java.util.regex.Pattern;



public class ViewerInfo extends JavaExtension{
   private String viewerName = null;
   private Vector<String> viewerDeps = new Vector<String>();
  
   public String toString(){
     String res = "[ViewerInfo: viewername = " + viewerName + 
                         ", SecondoVersion = " + secondo_Major_Version + "."
                                               + secondo_Minor_Version + "."
                                               + secondo_SubMinor_Version +
                         ", JavaVersion = " + java_Major_Version + "."
                                               + java_Minor_Version  +
                         ", mainClass = "      + mainClass +
                         ", copyright = "      + copyright;
     res += "), files = (";
     for(int i=0;i<files.size();i++){
       if(i>0) res +=", ";
       res += files.get(i);
     }
     res += "), libDeps= (";
     for(int i=0;i<libDeps.size();i++){
       if(i>0) res +=", ";
       res += libDeps.get(i);
     }
     res +=")]";
     return res;
   }

 
  /** Creates a new ViewerINfo object from an XML 
    * decsription.
    **/
   public ViewerInfo(Node n){
      valid = readViewerInfo(n);
   }

   public boolean isValid(){
     return valid;
   }


   /** Checks wether all files are present in the zip file **/
   public boolean filesPresent(ZipFile f){
      Vector<String> names = new Vector<String>();
      names.add(mainClass);
      for(int i=0;i<files.size();i++){
         names.add(files.get(i).first);
      }
      for(int i=0;i<libDeps.size();i++){
        StringBoolPair entry = libDeps.get(i);
        if(entry.firstB){
           names.add(entry.firstS);
        }
      }
      return filesPresent(f,names);
   }


   private boolean readViewerInfo(Node n1){
     NodeList nl = n1.getChildNodes();
     for(int i=0;i<nl.getLength();i++){
        Node n = nl.item(i);
        String name = n.getNodeName();
        if(name.equals("Mainclass")){
           if(mainClass!=null){
              System.err.println("XML coruppted: only one Mainclass allowed.");
              return false;
           } 
           if(n.hasChildNodes()) {
              mainClass = n.getFirstChild().getNodeValue().trim(); 
           } else {
              System.err.println("XML corrupted: mainclass cannot be empty");
              return false;
           } 
           if(mainClass.endsWith(".class")){
              viewerName = mainClass.substring(0,mainClass.length()-6);
           } else if(mainClass.endsWith(".java")){
              viewerName = mainClass.substring(0,mainClass.length()-5);
           } else {
              System.err.println("Mainclas has to end with .java or .class");
              return false;
           }
        } else if(name.equals("Files")){
            readFiles(n);
        } else if(name.equals("Dependencies")){
            readDependencies(n);
        } else if(name.equals("Copyright")){
           if(copyright!=null){
              System.err.println("XML coruppted: only one copyright allowed.");
              return false;
           } 
           if(n.hasChildNodes()) {
              copyright = n.getFirstChild().getNodeValue().trim(); 
           } else {
              System.err.println("XML corrupted: copyright cannot be empty");
              return false;
           }  
            
        } else if(!name.equals("#text") && !name.equals("#comment")){
           System.out.println("Unknown node for a viewer "+ name);
        }
     }
     return checkValidity();
   }

   

  boolean checkValidity(){
   if(viewerName==null){
      System.err.println("vieweName missing");   
      return false;
   }
   if(secondo_Major_Version<0){
     System.err.println("version information missing");
     return false;
   }
   if(mainClass==null){
      System.err.println("mainclass missing");
      return false;
   }
   return true;
  }

  static boolean check(String secondoDir, Vector<ViewerInfo> infos){
    if(!checkConflicts(secondoDir,infos)){
       return false;  
    } 
    if(!checkDependencies(secondoDir,infos)){
      return false;
    }
    return true;
  }

  /** Checks for conflict between packages and conflict between pacjages 
    * and the installed system.
    **/ 
  static boolean checkConflicts(String secondoDir, Vector<ViewerInfo> infos){
    // first names of the viewers must be disjoint
    TreeSet<String> names = new TreeSet<String>();
    TreeSet<String> files = new TreeSet<String>();
    String s = File.separator;
    String mainDir = secondoDir+s+"Javagui"+s;
    for(int i=0;i<infos.size();i++){
       ViewerInfo info = infos.get(i);
       names.add(info.viewerName);
       // create filenames for all files to be installed
       // use relative filenames starting from the Javagui directory
       // mainclass
       String fn =  mainDir + "viewer" + s + info.mainClass;       
       if(files.contains(fn)){
         System.err.println("Conflict: File " + fn + " found twice");
         return false;
       } 
       files.add(fn);
       // files
       for(int j=0;j<info.files.size();j++){
          StringPair pair = info.files.get(j);
          fn = mainDir +"viewer"+s+pair.second.replaceAll("/",s) + s + pair.first; 
          if(files.contains(fn)){
            System.err.println("Conflict: File " + fn + " found twice");
            return false;
          }
          files.add(fn);
       }
       // libDeps which should be installed
       for(int j=0;j<info.libDeps.size();j++){
         StringBoolPair e = info.libDeps.get(j);
         if(e.firstB){ // lib provided by the module
            fn = mainDir + "lib" + s + e.secondS.replaceAll("/",s) + s +  e.firstS;
            if(files.contains(fn)){
              System.err.println("Conflict: File " + fn + " found twice");
              return false;
            }
            files.add(fn);
         } 
       }
    }
    if(names.size()!=infos.size()){
      System.err.println("Viewer found twice");
      return false;
    }
    // the packages are pairwise conflict free

    // check for already installed files
    Iterator<String> it = files.iterator();
    while(it.hasNext()){
      String fn = it.next();
      File f = new File(fn);
      if(f.exists()){
         System.err.println("try to install file " + fn +" which is already installed");
         return false;
      }
    }
    return true;
  } // checkConflicts#



  /** Checks whether the dependencies are fullfilled */
  static boolean checkDependencies(String secondoDir, Vector<ViewerInfo> infos){
      // extract the viewer names
     TreeSet<String> names = new TreeSet<String>();
     for(int i=0;i<infos.size();i++){
       names.add(infos.get(i).viewerName);
       if(!infos.get(i).checkJavaVersion()){
          return false;
       }
     }

     String s = File.separator;
     String mainDir = secondoDir+s+"Javagui"+s;
     for(int i=0;i<infos.size();i++){
        ViewerInfo info = infos.get(i);
        if(!info.checkLibDeps(secondoDir)){
           return false;
        }
        for(int j=0;j<info.viewerDeps.size();j++){
          String p = info.viewerDeps.get(j);
          if(!names.contains(p)){
              String fn = mainDir +"viewer"+s+ p+".java"; 
              File f = new File(fn);
              if(!f.exists()){
                System.err.println("Viewer "+ info.viewerName +" requires the viewer "
                                 + p+  " which is not present.");
                return false;
              }
          }
        }
     }
   return true;
  }

 
  private boolean addToConfigFile(File f){
     System.out.println("Add entry to configuration file" + f );
     BufferedReader in = null;
     PrintWriter out = null;
     try{
        in = new BufferedReader(new FileReader(f));
        String content = "";
        boolean done = false;
        while(in.ready()){
          String line = in.readLine();
          TreeSet<String> viewers = new TreeSet<String>();
          if(!line.matches("\\s*KNOWN_VIEWERS\\s.*")){
            content += line +"\n";
          } else {
            String V = line.replaceAll("^\\s*KNOWN_VIEWERS\\s*","");
            StringTokenizer st = new StringTokenizer(V);
            while(st.hasMoreTokens()){
              viewers.add(st.nextToken());
            }
            if(viewers.contains(viewerName)){
              System.out.println("Viewer " + viewerName + " already kown");
            } else {
              line += " "+viewerName;
            }
            content += line +"\n";
          }
       }
       try{in.close();} catch(Exception e){}
       in = null;
       out = new PrintWriter(new FileOutputStream(f));
       out.print(content);
     } catch(Exception e){
       e.printStackTrace();
       System.err.println("problem in modifying config file");
       return  false;
     } finally {
       if(in!=null){
         try{in.close();}catch(Exception e){}
       }
       if(out!=null){
         try{out.close();}catch(Exception e){}
       }
     }
     return true;

  }

  private boolean addToStartScript(File f){
    // check whether the file is to mofify, i.e. if libs are required
    Vector<String> libFlags = new Vector<String>();
    for(int i=0;i<libDeps.size();i++){
        StringBoolPair p = libDeps.get(i);
        if(p.secondB){
           String flag = "lib" + File.separator;
           if(!p.secondS.equals(".")){
               flag += p.secondS + File.separator;
           }
           flag += p.firstS;
           libFlags.add(flag);
        }
    }
    if(libFlags.size()<1){ // no libraries are required
      return true;
    }
    
    Vector<String> remainingFlags = new Vector<String>();

    BufferedReader in = null;
    PrintWriter out = null;
    try{
      String content ="";
      in  = new BufferedReader(new FileReader(f));
      while(in.ready()){
        String line = in.readLine();
        if(!line.startsWith("CP=")){
           content+=line + "\n";
        } else {
           // analyse String for already used libraries
           String tmp = line.substring(3).trim(); 
           Pattern p = Pattern.compile("\\$S");
           String[] libs = p.split(tmp);
           TreeSet<String> usedLibs = new TreeSet<String>();
           for(int i=0;i<libs.length;i++){
              usedLibs.add(libs[i]);
           }

           for(int i=0;i<libFlags.size();i++){
               if(usedLibs.contains(libFlags.get(i)) || usedLibs.contains("\""+libFlags.get(i)+"\"" )){
                   System.err.println("lib " + libFlags.get(i) +" already used");
               } else {
                 remainingFlags.add(libFlags.get(i));
               }
           }   
           if(remainingFlags.size()==0){
              try{in.close();} catch(Exception e){}
              in = null;
              return true;
           } else {
             line = line.trim();
             for(int i=0;i<remainingFlags.size();i++){
                line += "$S\""+remainingFlags.get(i)+"\"";
             }
             content += line +"\n"; 
           }
        }
      }
      try{in.close();} catch(Exception e){}
     

       in = null;
       out = new PrintWriter(new FileOutputStream(f));
       out.print(content);
    } catch(Exception e){
      System.err.println("problem in modifying start script");
    } finally{
       if(in!=null){
         try{in.close();}catch(Exception e){}
       }
       if(out!=null){
         try{out.close();}catch(Exception e){}
       }
    }
    return true;
  }

  private boolean addToMakeFile(File make){
     BufferedReader in = null;
     PrintWriter out = null;
     try{
        in = new BufferedReader(new FileReader(make));
        String content = "";
        Vector<String> viewers = new Vector<String>();
        Vector<String> dirs = new Vector<String>();
        while(in.ready()){
           String line = in.readLine();
           if(line.matches("\\s*VIEWER_CLASSES\\s(:|\\+)=.*")){
             String V = line.replaceAll("\\s*VIEWER_CLASSES\\s(:|\\+)=\\s*","");
             StringTokenizer st = new StringTokenizer(V);
             while(st.hasMoreTokens()){
               viewers.add(st.nextToken());
             } 
             content += line +"\n";
           } else  if(line.matches("\\s*VIEWER_DIRS\\s(:|\\+)=.*")){
             String V = line.replaceAll("\\s*VIEWER_DIRS\\s(:|\\+)=\\s*","");
             StringTokenizer st = new StringTokenizer(V);
             while(st.hasMoreTokens()){
               dirs.add(st.nextToken());
             } 
             content += line +"\n";
           } else {
              content += line +"\n";
           }
        }
        if(!viewers.contains(viewerName)){
           content += "VIEWER_CLASSES += " + viewerName +".class\n";
        }
        for(int i=0;i<files.size();i++){
           String file = files.get(i).second;
           StringTokenizer st = new StringTokenizer(file,"/");
           if(st.countTokens()==1){
               String subdir = st.nextToken();
               if(!dirs.contains(subdir)){
                  content += "VIEWER_DIRS += " + subdir + "\n";
               }
           } 
        }
       
       try{ 
         in.close();
         in = null;
       } catch(Exception e){}
       out = new PrintWriter(new FileOutputStream(make));
       out.print(content);

     }catch(Exception e){
       System.err.println("error during updating " + make);
       return false; 
     } finally{
       if(in!=null){
         try{in.close();}catch(Exception e){}
       }
       if(out!=null){
         try{out.close();}catch(Exception e){}
       }
     }
     return true;
  }

  boolean modifyMakeFileInc(String secondoDir){
     // extract the lib names to add to makefile.inc
     Vector<StringBoolPair> libext = new Vector<StringBoolPair>();
     for(int i=0;i<libDeps.size();i++){
         StringBoolPair p = libDeps.get(i);
         if(p.secondB){
            libext.add(p);
         }
     }
     if(libext.size()==0){
        return true;
     }


     // appends libraries to makefile.inc
     String s = File.separator;
     File f = new File(secondoDir + s + "Javagui"+s+"makefile.inc");
     boolean ok = true;
     if(!f.exists()){
        System.err.println("Javagui/makefile.inc not found, check your Secondo installation");
        return false;
     }
     BufferedReader in = null;
     PrintWriter out = null;
     int pos = -1;
     try{
        in = new BufferedReader(new FileReader(f));
        String content = "";
        while(in.ready()){
           String line = in.readLine(); 
           if(!line.matches("\\s*JARS\\s*:=.*")){
              content += line +"\n";
           } else {
              content += line +"\n";
              pos = content.length();
              for(int i=libext.size()-1;i>=0;i--){
                 if(line.matches(".*"+libext.get(i).firstS+"\\s*")){
                   System.out.println("Lib " + libext.get(i).firstS+"already included in makefile.inc");
                   libext.remove(i);
                 }
              } 
           }
        }

        try{in.close(); in=null;}catch(Exception e){}

        if(libext.size()==0){
            return true;
        }
        String block = "";
        for(int i=0;i<libext.size();i++){
           StringBoolPair p = libext.get(i);
           String subdir = p.secondS.equals(".")?"":p.secondS;
           block += "JARS := $(JARS):$(LIBPATH)/"+subdir+p.firstS+"\n";
        }
        if(pos<0){
          content += block; 
        } else {
           String p1 = content.substring(0,pos);
           String p2 = content.substring(pos,content.length());
           content = p1+block+p2;
        }
        out = new PrintWriter(new FileOutputStream(f));
        out.print(content);
     }catch(Exception e){
       e.printStackTrace();
       ok = false;
     }finally{
       if(in!=null) try{in.close();}catch(Exception e){}
       if(out!=null) try{out.close();}catch(Exception e){}
     }
     return ok;
  }


  public boolean install(String secondoDir, String ZipFileName){
    // copy the files
     ZipFile f = null;
     String s = File.separator;
     String guiDir = secondoDir + s + "Javagui"+ s;
     String viewerDir = guiDir+"viewer"+s;
     String libDir = guiDir+"lib"+s;
     try{
       f = new ZipFile(ZipFileName);
       // copy mainClass
       String fn = viewerDir + mainClass; 
       copyZipEntryToFile(new File(fn), f, f.getEntry(mainClass));
       // copy Files
       for(int i=0; i< files.size(); i++){
         StringPair pair = files.get(i);
         fn = viewerDir + pair.second.replaceAll("/",s) + s + pair.first;
         copyZipEntryToFile(new File(fn), f, f.getEntry(pair.first));
       }
       // copy libraries
       for(int i=0;i<libDeps.size();i++){
          StringBoolPair ld = libDeps.get(i);
          if(ld.firstB){ // lib provided in zip file
            fn = libDir + ld.secondS.replaceAll("/",s) + s +  ld.firstS; 
            copyZipEntryToFile(new File(fn), f, f.getEntry(ld.firstS));
          }
       }
       
       File configFile = new File(guiDir+"gui.cfg.example");
       if(!configFile.exists()){
            System.err.println(configFile.toString()+" not found, please check your Secondo installation");
       }
       if(!addToConfigFile(configFile)){
          return false;
       }
       configFile = new File(guiDir +"gui.cfg");
       if(configFile.exists()){
         if(!addToConfigFile(configFile)){
            return false;
         }
       }
      
       File script = new File(guiDir+"sgui");
       if(!addToStartScript(script)){
         return false;
       }

       File make = new File(viewerDir + "makefile.viewers.example");
       if(!make.exists()){
         System.err.println("File " + make + " not found, check your Secondo installation");
         return false;
       }
       if(!addToMakeFile(make)){
          return false;
       }
       make = new File(viewerDir + "makefile.viewers");
       if(make.exists()){
          addToMakeFile(make);
       }
       modifyMakeFileInc(secondoDir);

     } catch(Exception e){
        e.printStackTrace();
        return false;
     } finally{
       if(f!=null){
         try{f.close();}catch(Exception e){}
       }
     }
     return true;  
  }

  


} // class ViewerInfo

