// $Id: Error.java,v 1.1 2002/11/09 09:13:17 jz Exp $

package sos.spooler;

/**
 * Überschrift:
 * Beschreibung:
 * Copyright:     Copyright (c) 2001, SOS GmbH Berlin
 * Organisation:  SOS GmbH Berlin
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version 1.0
 */

public class Error extends Idispatch
{
    private                     Error                       ( long idispatch )                      { super(idispatch); }

    public boolean              is_error                    ()                                      { return boolean_com_call( "<is_error" ); }
    public String               code                        ()                                      { return (String)com_call( "<code"     ); }
    public String               text                        ()                                      { return (String)com_call( "<text"     ); }
}
