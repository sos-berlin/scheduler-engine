// $Id: Task.java,v 1.6 2004/03/26 16:15:32 jz Exp $

package sos.spooler;

/**
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.6 $
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
    
    /** Mehrere Verzeichnisnamen sind durch Semikolon getrennt */
    public String           changed_directories ()                                  { return (String)       com_call( "<changed_directories"            ); }

    public void             add_pid             ( int pid )                         {                       com_call( "add_pid", pid                    ); }
    public void             add_pid             ( int pid, double timeout_seconds ) {                       com_call( "add_pid", new Integer(pid), new Double(timeout_seconds) ); }

    public void             remove_pid          ( int pid )                         {                       com_call( "remove_pid", pid                 ); }
}
