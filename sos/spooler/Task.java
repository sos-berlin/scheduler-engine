// $Id: Task.java,v 1.1 2002/11/09 09:13:18 jz Exp $

package sos.spooler;

/**
 * Überschrift:
 * Beschreibung:
 * Copyright:     Copyright (c) 2002, SOS GmbH Berlin
 * Organisation:  SOS GmbH Berlin
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version 1.0
 */

public class Task extends Idispatch
{
    private                 Task                ( long idispatch )                  { super(idispatch); }
    
  //public Object_set       object_set
    
    public void         set_error               ( String text )                     {                       com_call( ">error", text                    ); }
    public Error            error               ()                                  { return (Error)        com_call( "<error"                          ); }

    public Job              job                 ()                                  { return (Job)          com_call( "<job"                            ); }
    
    public Variable_set     params              ()                                  { return (Variable_set) com_call( "<params"                         ); }
    
    public void         set_result              ( String value )                    {                       com_call( ">result", value                  ); }
    public String           result              ()                                  { return (String)       com_call( "<result"                         ); }
    
    public boolean          wait_until_terminated()                                 { return        boolean_com_call( "wait_until_terminated"           ); }
    public boolean          wait_until_terminated( double wait_seconds )            { return ( (Boolean)    com_call( "wait_until_terminated", new Double(wait_seconds) ) ).booleanValue(); }
    
    public void         set_repeat              ( double seconds )                  {                       com_call( ">repeat", seconds                ); }
    
    public Thread           thread              ()                                  { return (Thread)       com_call( "<thread"                         ); }
    
    public void             end                 ()                                  {                       com_call( "end"                             ); }
    
    public void         set_history_field       ( String name, String value )       {                       com_call( ">history_field", name, value     ); }
    
    public int              id                  ()                                  { return            int_com_call( "<id"                             ); }
    
    public void         set_delay_spooler_process( double seconds )                 {                       com_call( ">delay_spooler_process", seconds ); }
    public void         set_delay_spooler_process( String hhmm_ss )                 {                       com_call( ">delay_spooler_process", hhmm_ss ); }
    
    public void         set_close_engine        ( boolean close_after_task )        {                       com_call( ">close_engine", close_after_task ); }
    
    public Order            order               ()                                  { return (Order)        com_call( "<order"                          ); }
}
