// $Id: Error.java,v 1.4 2004/07/12 17:59:49 jz Exp $

package sos.spooler;

/**
 * Fehlercode und -text.
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.4 $
 */

public class Error extends Idispatch
{
    private                     Error                       ( long idispatch )                      { super(idispatch); }

    
    /** @return true, wenn es sich um einen Fehler handelt. 
     */
    
    public boolean              is_error                    ()                                      { return boolean_com_call( "<is_error" ); }
    
    
    
    /** @return Fehlercode
     */
    
    public String               code                        ()                                      { return (String)com_call( "<code"     ); }
    
    
    /** @return Fehlertext
     */
    public String               text                        ()                                      { return (String)com_call( "<text"     ); }
}
