// $Id

package sos.spooler;

/**
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.4 $
 */

public class Spooler extends Idispatch
{
    private                 Spooler             ( long idispatch )                  { super(idispatch); }

    public Log              log                 ()                                  { return (Log)          com_call( "<log"                            ); }
    public String           id                  ()                                  { return (String)       com_call( "<id"                             ); }
    public String           param               ()                                  { return (String)       com_call( "<param"                          ); }
  //public IDispatch        script              ()                                  { return (Idispatch)    com_call( "<script"                         ); }
    public Job              job                 ( String job_name )                 { return (Job)          com_call( "<job", job_name                  ); }
    public Variable_set     create_variable_set ()                                  { return (Variable_set) com_call( "create_variable_set"             ); }
    public String           include_path        ()                                  { return (String)       com_call( "<include_path"                   ); }
    public String           log_dir             ()                                  { return (String)       com_call( "<log_dir"                        ); }
    public void             let_run_terminate_and_restart()                         {                       com_call( "let_run_terminate_and_restart"   ); }
    public Variable_set     variables           ()                                  { return (Variable_set) com_call( "<variables"                      ); }
    public void         set_var                 ( String name, String value )       {                       com_call( ">var", name, value               ); }
    public String           var                 ( String name )                     { return (String)       com_call( "<var", name                      ); }
    public String           db_name             ()                                  { return (String)       com_call( "<db_name"                        ); }
    public Job_chain        create_job_chain    ()                                  { return (Job_chain)    com_call( "create_job_chain"                ); }
    public void             add_job_chain       ( Job_chain job_chain )             {                       com_call( "add_job_chain", job_chain        ); }
    public Job_chain        job_chain           ( String name )                     { return (Job_chain)    com_call( "<job_chain", name                ); }
    public Order            create_order        ()                                  { return (Order)        com_call( "create_order"                    ); }
    public boolean          is_service          ()                                  { return        boolean_com_call( "<is_service"                     ); }
}
