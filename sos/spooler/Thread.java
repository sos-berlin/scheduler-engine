// $Id: Thread.java,v 1.1 2002/11/09 09:13:18 jz Exp $

package sos.spooler;

/**
 * Überschrift:
 * Beschreibung:
 * Copyright:     Copyright (c) 2002, SOS GmbH Berlin
 * Organisation:  SOS GmbH Berlin
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version 1.0
 */


public class Thread extends Idispatch
{
    private                 Thread              ( long idispatch )                  { super(idispatch); }

    public Log              log                 ()                                  { return (Log)      com_call( "<log"            ); }
  //public Script           script              ();
    public String           include_path        ()                                  { return (String)   com_call( "<include_path"   ); }
    public String           name                ()                                  { return (String)   com_call( "<name"           ); }
}
