// $Id: Job_impl.java,v 1.5 2003/04/21 19:20:08 jz Exp $

package sos.spooler;

/**
 * Oberklasse für die Implementierung eines Jobs.
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.5 $
 */

public class Job_impl
{
    /** Wird bei use_engine="task" nur einmal für mehrere Jobläufe gerufen. 
      * Gegenstück ist {@link #spooler_exit()}.
      * @return false stoppt den Job.
      */
    public boolean  spooler_init        ()      throws Exception  { return true; }


    /** Wir als allerletzte Methode gerufen, bevor das Java-Objekt verworfen wird. */
    public void     spooler_exit        ()      throws Exception  {}


    /** Wird zu Beginn eines Joblaufs gerufen.
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
      * @return bei order="no": false beendet den Joblauf.
      * bei order="true": false versetzt den Auftrag in den Fehlerzustand (s. {@link Job_chain_node}).
      */

    public boolean  spooler_process     ()      throws Exception  { return false; }


    /** Wird als letzte Funktion eines fehlerlosen Joblaufs gerufen.
      */

    public void     spooler_on_error    ()      throws Exception  {}


    /** Wird als letzte Funktion eines Joblaufs gerufen, wenn ein Fehler aufgetreten ist. */
    public void     spooler_on_success  ()      throws Exception  {}


    /** Zum Protokollieren */
    public Log      spooler_log;

    
    /** Objekt zum Joblauf */
    public Task     spooler_task;

    public Job      spooler_job;

    public Thread   spooler_thread;

    public Spooler  spooler;
}
