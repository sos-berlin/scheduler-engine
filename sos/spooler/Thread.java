// $Id: Thread.java,v 1.3 2002/11/14 12:34:50 jz Exp $

package sos.spooler;

/**
 * Spooler-Thread
 * Copyright (c) 2002, SOS GmbH Berlin
 * Organisation:  SOS GmbH Berlin
 * @version $Revision: 1.3 $
 */


public class Thread extends Idispatch
{
    private                 Thread              ( long idispatch )                  { super(idispatch); }

    /** {@link Log} des Threads. */
    public Log              log                 ()                                  { return (Log)      com_call( "<log"            ); }

  //public Script           script              ();

    public String           include_path        ()                                  { return (String)   com_call( "<include_path"   ); }

    public String           name                ()                                  { return (String)   com_call( "<name"           ); }
}
