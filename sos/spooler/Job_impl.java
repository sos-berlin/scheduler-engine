// $Id: Job_impl.java,v 1.1 2002/11/09 09:13:17 jz Exp $

package sos.spooler;

/**
 * Überschrift:
 * Beschreibung:
 * Copyright:     Copyright (c) 2002, SOS GmbH Berlin
 * Organisation:  SOS GmbH Berlin
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version 1.0
 */

public class Job_impl
{
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
