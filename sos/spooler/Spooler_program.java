// $Id

package sos.spooler;

/**
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.1 $
 */

public class Spooler_program
{
    public                  Spooler_program         ( String parameters )               { construct( parameters ); }

    private native void     construct               ( String parameters );


    static
    {
        System.loadLibrary( "spooler" );
    }
}
