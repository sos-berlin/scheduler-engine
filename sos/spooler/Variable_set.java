// $Id: Variable_set.java,v 1.6 2004/06/11 11:13:56 jz Exp $

package sos.spooler;

/**
 * Variablenmenge.
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.6 $
 */

public class Variable_set extends Idispatch
{
    private                 Variable_set        ( long idispatch )                  { super(idispatch); }

    public void         set_var                 ( String name, String value )       {                       com_call( ">var", name, value       ); }
    public String           var                 ( String name )                     { return (String)       com_call( "<var", name              ); }


    /** @return Liefert die Anzahl der Variablen.
      */

    public int              count               ()                                  { return            int_com_call( "<count"                  ); }
  
  //public Dom              dom                 ()
  
  
  //public Variable_set     clone               ()                                  { return (Variable_set) com_call( "clone"                   ); }
    
    public void             merge               ( Variable_set vars )               {                       com_call( "merge", vars             ); }


    /** Parameter ist ein XML-Dokument folgender DTD:
      * <!ELEMENT variable_set ( variable* )>
      * <!ELEMENT variable EMPTY>
      * <!ATTLIST variable name CDATA #REQUIRED>
      * <!ATTLIST variable value CDATA #REQUIRED>
      *
      * Die Variablen im XML-Dokument werden dem Variable_set hinzugefügt. 
      * Vorhandene Variablen werden überschrieben.
      */

    public void         set_xml                 ( String xml_text )                 {                       com_call( ">xml", xml_text          ); }


    /** Liefert die Variablenmenge als XML-Dokument, wie in {@link #set_xml(String)} beschrieben.
      * Das XML-Dokument kann {@link #set_xml(String)} übergeben werden.
      */

    public String           xml                 ()                                  { return (String)       com_call( "<xml"                    ); }
}
