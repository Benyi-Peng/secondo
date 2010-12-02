

import java.net.URL;
import java.io.File;
import java.io.InputStream;
import java.io.FileOutputStream;
import java.util.HashMap;
import java.util.Queue;
import java.util.LinkedList;
import java.util.Vector;
import java.util.TreeSet;
import java.util.Collection;
import java.util.Iterator;

interface DownloadObserver{
  public void downloadStateChanged(DownloadEvent evt);
}


class PlannedDownload{
  public PlannedDownload(URL url, File targetFile, DownloadObserver ob){
     this.url =url;
     this.targetFile = targetFile;
     canceled = false;
     observers= new TreeSet<DownloadObserver>();
     observers.add(ob);
  }

  public PlannedDownload(PlannedDownload src){
    this.url = src.url;
    this.targetFile = src.targetFile;
    this.canceled=src.canceled;
    this.observers = new TreeSet<DownloadObserver>();
    this.observers.addAll(src.observers);
  }



  public boolean addObserver(DownloadObserver observer){
    if(canceled){
       return false;
    }  else {
       observers.add(observer);
       return  true;
    }
  }

  public void cancel(){
      canceled=true;
  }

  public URL getURL(){
     return url;
  }

  public File getTargetFile(){
     return targetFile;
  }

  public boolean isCanceled(){
    return canceled;
  }

  protected void informListeners(DownloadEvent evt){
    Iterator<DownloadObserver> it = observers.iterator();
    while(it.hasNext()){
       it.next().downloadStateChanged(evt);
       it.remove();
    }
  }


  protected URL url;
  protected File  targetFile;
  private TreeSet<DownloadObserver> observers;
  protected boolean canceled;
}


enum DownloadState {CANCEL,DONE,BROKEN};

class DownloadEvent{
  public DownloadEvent( Object source, DownloadState state, Exception ex){
     this.source = source;
     this.state = state;
     this.ex = ex;
  }

  public Object getSource(){
     return source;
  }

  public DownloadState getState(){
     return state;
  }

  public Exception getException(){
    return ex;
  } 

  private Object source;
  private DownloadState state;
  private Exception ex;
}


class ActiveDownload extends PlannedDownload implements Runnable{



  public ActiveDownload(URL url, File targetFile, DownloadObserver observer){
     super(url,targetFile, observer);
  }

  public ActiveDownload(PlannedDownload pd){
     super(pd);
  }


  public void start(){
     new Thread(this).start();
  }


  public void run(){
    InputStream in = null;
    FileOutputStream out=null;
    try{
       in = url.openStream();
       out = new FileOutputStream(targetFile);
       byte[] buffer = new byte[256];
       int size = -1;
       do{
          size = in.read(buffer);
          out.write(buffer,0,size); 
       }while(!canceled && (size>=0));

       in.close();
       out.close();
       if(canceled){
         informListeners(new DownloadEvent(this, DownloadState.CANCEL, null));
         return;
       }
       // download complete
       informListeners(new DownloadEvent(this,DownloadState.DONE, null));
    } catch(Exception e){
       if(in!=null){
         try{
           in.close();
         }catch(Exception ein){}
       }
       if(out!=null){
         try{
           out.close();
         }catch(Exception eout){}
       }
       informListeners(new DownloadEvent(this,DownloadState.BROKEN, e));
    }
  }
}

class InvalidArgumentException extends Exception{
  public  InvalidArgumentException(String message) {
    this.message=message;
  }

  private String message;
}

public class DownloadManager implements DownloadObserver{


  /** Creates a new DownloadManager. 
   **/
  public void DownloadManager(File tmpDirectory, int maxDownloads) throws InvalidArgumentException{
     if(!tmpDirectory.isDirectory()){
         throw new InvalidArgumentException("tmpdirectory must be an directiry");
     }
     if(!tmpDirectory.exists()){
         tmpDirectory.mkdirs();
     }
     if(!tmpDirectory.canRead() || !tmpDirectory.canWrite()){
         throw new InvalidArgumentException("access to "+tmpDirectory.getName()+" not permitted");
     }
     // successful tmpdir can be used
     rootDir =tmpDirectory;
     this.maxDownloads = Math.max(1,maxDownloads);

     plannedDownloads = new HashMap<URL, PlannedDownload>();
     activeDownloads = new HashMap<URL, ActiveDownload>();
     plannedQueue = new LinkedList<URL>();
  }


  /** retrieves a file.
   *  If this url was already downloaded, the File containing the content of
   * the url is returned. Otherwise the return value is null. If the download state
   * changes (url does not exists, download complete etc.), the given observer is
   * informed about this change. To really get the file, just call getFile again.
   * Note: The file can become invalid if the clearCache or removeUrl function
   * is called.
   */
  public synchronized File getURL(URL url, DownloadObserver ob){
    File f = computeFile(url);
    if(f.exists()){
      return f;
    }

    // insert observer is there is a download for that url 
    if(insertObserver(url,ob)){ 
      return null;
    }

    if( (plannedDownloads.size()>0) || (activeDownloads.size()>=maxDownloads)){
       // all download slots are used. So, the download is just planned
       plannedDownloads.put(url, new PlannedDownload(url,f, ob));
       plannedQueue.offer(url);
       return null;
    }
    // the is a free download slot, create a new download for the url
    ActiveDownload dl = new ActiveDownload(url, f, this);
    dl.addObserver(ob);
    activeDownloads.put(url,dl);
    dl.start();
    return null;
  }


  /** Updates an downloaded url.
    * The file is actually updated if the file is not present,
    * force is set to true, the file content behind the urls does
    * not provide time information, or the content on the web server
    * is newer than the file.
    * If the file is up to date (or cannot be downloaded), the observer is
    * informed about this. To get the updated file, call the getFile method.
    * @param url: url to be updated
    * @param force: flag to force an update even is the local file seems to be up to date
    * @param ob: observer which should be informed about the progress of the download
    **/
  public void updateURL(URL url, boolean force, DownloadObserver ob){
    if(insertObserver(url,ob)){
       return;
    }
    File f = computeFile(url);
    // file not present, just start the normal download
    if(!f.exists()){
      getURL(url,ob);
      return;
    }
    // file is present // do some more complicated stuff
      File tmpFile = computeTmpFile(url);

      // idea start a new download into a temporarly file
      // assign another observer to that download
      // if the download is finished successful, move the new file to the 
      // file to be updated 
      // if the downloads breaks down, remove the temp file 
      // in each case inform ob about the new state.
      //TODO:...
 }



  
  /** Removes a cached file, or cancels a download for a URL.
    * If the download is canceled, the observer is informed about that.
    **/
  public boolean removeURL(URL url){
     PlannedDownload pd = plannedDownloads.get(url);
     if(pd!=null){
        pd.cancel();
        plannedDownloads.remove(url);
        return true;
     }
     ActiveDownload ad = activeDownloads.get(url);
     if(ad!=null){
        ad.cancel();
        activeDownloads.remove(url);
        return true; 
     }
     File f = computeFile(url);
     if(f.exists()){
        f.delete();
        return true;
     }
     // url was not managed
     return  false; 
  }


  /** cancels all active and inactive downloads informing the related observers **/
  public void cancelDownloads(){
     Collection<PlannedDownload> cpd = plannedDownloads.values();
     plannedDownloads.clear();     
     Iterator<PlannedDownload> itpd = cpd.iterator();
     while(itpd.hasNext()){
        PlannedDownload pd = itpd.next();
        itpd.remove();
        pd.cancel(); 
     }

     Collection<ActiveDownload> cad = activeDownloads.values();
     activeDownloads.clear();
     Iterator<ActiveDownload> itad = cad.iterator();
     while(itad.hasNext()){
        ActiveDownload ad = itad.next();
        itad.remove();
        ad.cancel(); 
     }
  }

  /** cancels all downloads and removes all files located below the tmpdirectory **/
  public void clearChache() {
     cancelDownloads();
     deleteDirContent(rootDir);
  }


  /** reaction to finished downloads **/
  public void downloadStateChanged(DownloadEvent evt){
    ActiveDownload ad = (ActiveDownload) evt.getSource();
    activeDownloads.remove(ad.getURL());
    while(plannedQueue.size()>0){
       URL url = (URL) plannedQueue.poll();
       PlannedDownload pd = plannedDownloads.get(url);
       if(pd!=null){
          plannedDownloads.remove(url);
          activate(pd);
          return;
       }
    }
  }

  /** activates a planned download **/
  private void activate(PlannedDownload pd){
     ActiveDownload ad = new ActiveDownload(pd);
     activeDownloads.put(ad.getURL(), ad);
     ad.start();
  }



  /* Compute the file from the rootDirectiry and the URL */
  private File computeFile(URL url, File rootDir){
    try{
      File p = new File(rootDir,url.getProtocol());
      File ph = new File(p,url.getHost());
      int port = url.getPort();
      if(port<0){
          port = url.getDefaultPort();
      }   
      File php = new File(ph,""+port);
      File phpf = new File(php,url.getFile());
      return phpf.getCanonicalFile();
     } catch(Exception e){ 
       e.printStackTrace();
       return new File("Error");
    }   
  }

  private File computeFile(URL url){
     return computeFile(url,rootDir);
  }

  private File computeTmpFile(URL url){
      return computeFile(url, new File(rootDir,"temp"+File.separator));
  }


  /** Inserts a new Observer into existing download for the specified url.
    * If there is no download for this url, the result will be false.
    * Otherwise true.
    **/
  private boolean insertObserver(URL url, DownloadObserver ob){
    PlannedDownload pd = plannedDownloads.get(url);
    if(pd!=null){
       pd.addObserver(ob);
       return true;
    }
    ActiveDownload ad = activeDownloads.get(url);
    if(ad!=null){
       ad.addObserver(ob);
       return true;
    }
    return false;
  }

  /** delete a directory and all contained stuff **/
  private boolean deleteDirectory(File root){
     if(deleteDirContent(root)){
           return root.delete();
     } else {
         return false;
     }
  }

  /** deletes the content of a directory **/
  private boolean deleteDirContent(File root){
    if(!root.isDirectory()){
       return false;
    }
    try{
      File[] content = root.listFiles();
      for(int i=0;i!=content.length;i++){
        if(    !content[i].getName().equals(".") 
            && !content[i].getName().equals("..")){
           if(content[i].isDirectory()){
              if(!deleteDirContent(content[i])){
                return false;
              }
           }
           content[i].delete();
        }
      }
    } catch(Exception e){
        e.printStackTrace();
        return false;
    }
    return true;
  }





  private File rootDir;
  private int maxDownloads;
  private HashMap<URL, ActiveDownload> activeDownloads;
  private HashMap<URL, PlannedDownload> plannedDownloads;
  private Queue plannedQueue;
  
}
