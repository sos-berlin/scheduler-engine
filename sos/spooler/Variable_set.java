// $Id: Variable_set.java,v 1.4 2003/03/31 11:32:54 jz Exp $

package sos.spooler;

/**
 * Variablenmenge.
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.4 $
 */

public class Variable_set extends Idispatch
{
    private                 Variable_set        ( long idispatch )                  { super(idispatch); }

    public void         set_var                 ( String name, String value )       {                       com_call( ">var", name, value       ); }
    public String           var                 ( String name )                     { return (String)       com_call( "<var", name              ); }

    public int              count               ()                                  { return            int_com_call( "<count"                  ); }
  
  //public Dom              dom                 ()
  
  
  //public Variable_set     clone               ()                                  { return (Variable_set) com_call( "clone"                   ); }
    
    public void             merge               ( Variable_set vars )               {                       com_call( "merge", vars             ); }

    public void         set_xml                 ( String xml_text )                 {                       com_call( ">xml", xml_text          ); }
    public String           xml                 ()                                  { return (String)       com_call( "<xml"                    ); }
}
