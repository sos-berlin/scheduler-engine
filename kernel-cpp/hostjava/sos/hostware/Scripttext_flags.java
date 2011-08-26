// $Id: Scripttext_flags.java 11346 2005-03-24 10:00:11Z jz $

package sos.hostware;


class Scripttext_flags
{
    private Scripttext_flags( int i ) { value = i; }

    static final Scripttext_flags none         = new Scripttext_flags(          0 );
    static final Scripttext_flags isvisible    = new Scripttext_flags( 0x00000002 );
    static final Scripttext_flags isexpression = new Scripttext_flags( 0x00000020 );
    static final Scripttext_flags ispersistent = new Scripttext_flags( 0x00000040 );

    public int value = 0;
}
