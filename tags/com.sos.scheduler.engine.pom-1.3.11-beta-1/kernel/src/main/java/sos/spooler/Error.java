// $Id: Error.java 3946 2005-09-26 08:52:01Z jz $

package sos.spooler;

/*+
 * Fehlercode und -text.
 */
/** 
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 3946 $
 */

public class Error extends sos.spooler.Idispatch
{
    private                     Error                       ( long idispatch )                      { super(idispatch); }

    
    /*+ Liefert true, wenn es ein Fehler ist.
     */
    
    public boolean              is_error                    ()                                      { return boolean_com_call( "<is_error" ); }
    
    
    
    /*+ Der Fehlercode
     */
    
    public String               code                        ()                                      { return (String)com_call( "<code"     ); }
    
    
    /*+ Der Fehlertext (mit Fehlercode)
     */
    public String               text                        ()                                      { return (String)com_call( "<text"     ); }
}
