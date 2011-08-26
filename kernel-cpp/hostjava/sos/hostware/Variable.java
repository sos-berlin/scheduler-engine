// $Id: Variable.java 11346 2005-03-24 10:00:11Z jz $

package sos.hostware;

/**
 * @author Joacim Zschimmer
 */

public class Variable
{
    public native String            value                       ();
    public native void          set_value                       ( String value );

    public native String            value                       ( int index );
    public native void          set_value                       ( int index, String value );

    public native void              dim                         ( int size );

    public native String            name                        ();


    private int                     my_data;
}

