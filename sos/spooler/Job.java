// $Id: Job.java,v 1.1 2002/11/09 09:13:17 jz Exp $

package sos.spooler;

/**
 * Überschrift:
 * Beschreibung:
 * Copyright:     Copyright (c) 2002, SOS GmbH Berlin
 * Organisation:  SOS GmbH Berlin
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version 1.0
 */

public class Job extends Idispatch
{
    private                 Job                 ( long idispatch )                  { super(idispatch); }
    
    public void             start_when_directory_changed( String directory_name )                           { com_call( "start_when_directory_changed", directory_name ); }
    public void             start_when_directory_changed( String directory_name, String filename_pattern )  { com_call( "start_when_directory_changed", directory_name, filename_pattern ); }

    public void             clear_when_directory_changed()                          {                     com_call( "clear_when_directory_changed"  ); }

    public Task             start               ()                                  { return (Task)       com_call( "start"                         ); }
    public Task             start               ( Variable_set variables )          { return (Task)       com_call( "start", variables              ); }
    public Thread           thread              ()                                  { return (Thread)     com_call( "<thread"                       ); }
    public String           include_path        ()                                  { return (String)     com_call( "<include_path"                 ); }
    public String           name                ()                                  { return (String)     com_call( "<name"                         ); }
    public void             wake                ()                                  {                     com_call( "wake"                          ); }
    public void         set_state_text          ( String text )                     {                     com_call( ">state_text", text             ); }
    public String           title               ()                                  { return (String)     com_call( "<title"                        ); }
    public void         set_delay_after_error   ( int error_steps, double seconds ) {                     com_call( ">delay_after_error", seconds   ); }
    public void         set_delay_after_error   ( int error_steps, String hhmm_ss ) {                     com_call( ">delay_after_error", hhmm_ss   ); }
    public Order_queue      order_queue         ()                                  { return (Order_queue)com_call( "<order_queue"                  ); }
}
