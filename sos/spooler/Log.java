// $Id: Log.java,v 1.4 2003/02/25 20:33:49 jz Exp $

package sos.spooler;

/**
 * Protokollierung
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version 1.0
 */

public class Log extends Idispatch
{
    private                     Log                     ( long idispatch )                          { super(idispatch); }

    public void                 log                     ( int level, String line )                  { com_call( "log"   , new Integer(level), line ); }
    public void                 error                   ( String line )                             { com_call( "error" , line ); }
    public void                 warn                    ( String line )                             { com_call( "warn"  , line ); }
    public void                 info                    ( String line )                             { com_call( "info"  , line ); }
    public void                 debug                   ( String line )                             { com_call( "debug" , line ); }
    public void                 debug1                  ( String line )                             { com_call( "debug1", line ); }
    public void                 debug2                  ( String line )                             { com_call( "debug2", line ); }
    public void                 debug3                  ( String line )                             { com_call( "debug3", line ); }
    public void                 debug4                  ( String line )                             { com_call( "debug4", line ); }
    public void                 debug5                  ( String line )                             { com_call( "debug5", line ); }
    public void                 debug6                  ( String line )                             { com_call( "debug6", line ); }
    public void                 debug7                  ( String line )                             { com_call( "debug7", line ); }
    public void                 debug8                  ( String line )                             { com_call( "debug8", line ); }
    public void                 debug9                  ( String line )                             { com_call( "debug9", line ); }

    public Mail                 mail                    ()                                          { return (Mail)     com_call( "<mail"                   ); }

    public void             set_mail_on_error           ( boolean value )                           {                   com_call( ">mail_on_error", value   ); }
    public boolean              mail_on_error           ()                                          { return    boolean_com_call( "<mail_on_error"          ); }

    public void             set_mail_on_success         ( boolean value )                           {                   com_call( ">mail_on_success", value ); }
    public boolean              mail_on_success         ()                                          { return    boolean_com_call( "<mail_on_success"        ); }

    public void             set_mail_on_process         ( int steps )                               {                   com_call( ">mail_on_process", steps ); }
    public int                  mail_on_process         ()                                          { return        int_com_call( "<mail_on_process"        ); }

    public void             set_level                   ( int level )                               {                   com_call( ">level", level           ); }
    public int                  level                   ()                                          { return        int_com_call( "<level"                  ); }

    public String               filename                ()                                          { return (String)   com_call( "<filename"               ); }

    public void             set_new_filename            ( String filename )                         {                   com_call( ">new_filename", filename ); }
    public String               new_filename            ()                                          { return (String)   com_call( "<new_filename"           ); }

    public void             set_collect_within          ( double time )                             {                   com_call( ">collect_within", time   ); }
    public void             set_collect_within          ( String time )                             {                   com_call( ">collect_within", time   ); }
    public double               collect_within          ()                                          { return     double_com_call( "<collect_within"         ); }

    public void             set_collect_max             ( double time )                             {                   com_call( ">collect_max", time      ); }
    public void             set_collect_max             ( String time )                             {                   com_call( ">collect_max", time      ); }
    public double               collect_max             ()                                          { return     double_com_call( "<collect_max"            ); }
}
