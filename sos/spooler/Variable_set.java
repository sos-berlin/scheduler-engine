// $Id: Variable_set.java,v 1.1 2002/11/09 09:13:18 jz Exp $

package sos.spooler;

/**
 * Überschrift:
 * Beschreibung:
 * Copyright:     Copyright (c) 2001, SOS GmbH Berlin
 * Organisation:  SOS GmbH Berlin
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version 1.0
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
