// $Id: Error.java,v 1.2 2002/11/13 21:20:49 jz Exp $

package sos.spooler;

/**
 * Fehlercode und -text.
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.2 $
 */

public class Error extends Idispatch
{
    private                     Error                       ( long idispatch )                      { super(idispatch); }

    public boolean              is_error                    ()                                      { return boolean_com_call( "<is_error" ); }
    public String               code                        ()                                      { return (String)com_call( "<code"     ); }
    public String               text                        ()                                      { return (String)com_call( "<text"     ); }
}
