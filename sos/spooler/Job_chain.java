// $Id

/**
 * Jobkette
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.3 $
 */

package sos.spooler;

public class Job_chain extends Idispatch
{
    private                 Job_chain           ( long idispatch )                  { super(idispatch); }

    public void         set_name                ( String value )                    {                           com_call( ">name", value        ); }
    public String           name                ()                                  { return (String)           com_call( "<name"               ); }
    public int              order_count         ()                                  { return                int_com_call( "<order_count"        ); }
    public void             add_job             ( String jobname, String input_state, 
                                                  String output_state, String error_state ) {                   com_call( "add_job", jobname, input_state, output_state, error_state ); }
    public void             add_end_state       ( String state )                    {                           com_call( "add_end_state", state ); }
    public void             add_order           ( Order order )                     {                           com_call( "add_order", order    ); }
    public Job_chain_node   node                ( String state )                    { return (Job_chain_node)   com_call( "<node", state        ); }
    public Order_queue      order_queue         ( String state )                    { return (Order_queue)      com_call( "<order_queue", state ); }
}
