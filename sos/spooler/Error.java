// $Id$

package sos.spooler;

/**
 * Fehlercode und -text.
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision$
 */

public class Error extends Idispatch
{
    private                     Error                       ( long idispatch )                      { super(idispatch); }

    
    /** Liefert true, wenn es ein Fehler ist.
     */
    
    public boolean              is_error                    ()                                      { return boolean_com_call( "<is_error" ); }
    
    
    
    /** Der Fehlercode
     */
    
    public String               code                        ()                                      { return (String)com_call( "<code"     ); }
    
    
    /** Der Fehlertext (mit Fehlercode)
     */
    public String               text                        ()                                      { return (String)com_call( "<text"     ); }
}
