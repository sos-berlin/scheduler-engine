// $Id: Job_impl.java,v 1.2 2002/11/13 18:15:39 jz Exp $

package sos.spooler;

/**
 * Oberklasse für einen Spooler-Job.
 * Alle Methoden sind leer vorimplementiert.
 * Copyright (c) 2002, SOS GmbH Berlin
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.2 $
 */

public class Job_impl
{
    /** Wird bei use_engine="task" nur einmal für mehrere Jobläufe gerufen. 
      * Gegenstück ist {@link #spooler_exit()}.
      * @return false stoppt den Job.
      */
    public boolean  spooler_init        ()      { return true; }


    public void     spooler_exit        ()      {}
    public boolean  spooler_open        ()      { return true; }
    public void     spooler_close       ()      {}
    public boolean  spooler_process     ()      { return false; }
    public void     spooler_on_error    ()      {}
    public void     spooler_on_success  ()      {}


    public Log      spooler_log;
    public Task     spooler_task;
    public Job      spooler_job;
    public Thread   spooler_thread;
    public Spooler  spooler;
}
