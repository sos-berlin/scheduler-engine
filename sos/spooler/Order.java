// $Id: Order.java,v 1.3 2002/11/14 12:34:50 jz Exp $

/**
 * Auftrag
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.3 $
 */

package sos.spooler;

public class Order extends Idispatch
{
    private                 Order               ( long idispatch )                  { super(idispatch); }

    public void         set_id                  ( String value )                    {                   com_call( ">id", value          ); }
    public String           id                  ()                                  { return (String)   com_call( "<id"                 ); }
    
    public void         set_title               ( String value )                    {                   com_call( ">title", value       ); }
    public String           title               ()                                  { return (String)   com_call( "<title"              ); }
    
    public void         set_priority            ( int value )                       {                   com_call( ">priority", value    ); }
    public String           priority            ()                                  { return (String)   com_call( "<priority"           ); }
    
    public Job_chain        job_chain           ()                                  { return (Job_chain)com_call( "<job_chain"          ); }
    
    public Job_chain_node   job_chain_node      ()                                  { return (Job_chain_node)com_call( "<job_chain_node" ); }
    
    public void         set_job                 ( Job job )                         {                   com_call( ">job", job           ); }
    public void         set_job                 ( String job_name )                 {                   com_call( ">job", job_name      ); }
    public Job              job                 ()                                  { return (Job)      com_call( "<job"                ); }
    
    public void         set_state               ( String value )                    {                   com_call( ">state", value       ); }
    public String           state               ()                                  { return (String)   com_call( "<state"              ); }
    
    public void         set_state_text          ( String value )                    {                   com_call( ">state_text", value  ); }
    public String           state_text          ()                                  { return (String)   com_call( "<state_text"         ); }
    
    public void         set_payload             ( Object payload )                  {                   com_call( ">payload", payload   ); }
    public Object           payload             ()                                  { return            com_call( "<payload"            ); }
    
    public boolean          payload_is_type     ( String name )                     { return    boolean_com_call( "payload_is_type"     ); }
}
