// $Id: Variable_set.java,v 1.3 2002/11/14 12:34:50 jz Exp $

package sos.spooler;

/**
 * Variablenmenge.
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.3 $
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
}
