// $Id: Job_impl.java,v 1.6 2004/07/12 17:59:49 jz Exp $

package sos.spooler;

/**
 * Oberklasse für die Implementierung eines Jobs.
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.6 $
 */

public class Job_impl
{
    /** Der Scheduler ruft diese Methode nach dem Konstruktor und vor {@link #spooler_open()} genau einmal auf. 
      * Gegenstück ist {@link #spooler_exit()}. Die Methode ist geeignet, um die Task zu initialisieren 
      * (z.B. um eine Datenbank-Verbindung aufzubauen).
      * @return false stoppt den Job.
      */
    
    public boolean  spooler_init        ()      throws Exception  { return true; }

    

    /** Wir als allerletzte Methode gerufen, bevor das Java-Objekt verworfen wird. 
      * Hier kann z.B. eine Datenbank-Verbindung geschlossen werden. 
      */
    
    public void     spooler_exit        ()      throws Exception  {}


    
    /** Wird zu Beginn einer Task gerufen. 
      * Die Methode wird direkt nach {@link #spooler_init()} gerufen, es gibt derzeit keinen Unterschied.
      * Gegenstück ist {@link #spooler_close()}.
      * @return false beendet den Joblauf.
      */

    public boolean  spooler_open        ()      throws Exception  { return true; }


    
    /** Wird am Ende eines Joblaufs gerufen.
      * Gegenstück zu {@link #spooler_open()}.
      */

    public void     spooler_close       ()      throws Exception  {}


    /** Führt einen Jobschritt aus.
      * Gegenstück ist {@link #spooler_exit()}.
      * Die Default-Implementierung gibt false zurück.
      * 
      * @return bei &lt;job order="no">: false beendet den Joblauf.
      * Bei &lt;job order="true">: false versetzt den Auftrag in den Fehlerzustand (s. {@link Job_chain_node}).
      */

    public boolean  spooler_process     ()      throws Exception  { return false; }


    /** Wird als letzte Funktion eines Joblaufs gerufen, wenn ein Fehler aufgetreten ist 
      * (nach {@link #spooler_close()}, vor {@link #spooler_exit()}).
      */

    public void     spooler_on_error    ()      throws Exception  {}

    
    /** Wird als letzte Funktion einer fehlerlosen Task gerufen (nach {@link #spooler_close()}, vor {@link #spooler_exit()}).
      */

    public void     spooler_on_success  ()      throws Exception  {}


    /** Zum Protokollieren */
    public Log      spooler_log;

    
    /** Objekt zum Joblauf */
    public Task     spooler_task;

    
    public Job      spooler_job;

    
    //public Thread   spooler_thread;

    
    public Spooler  spooler;
}
