// $Id: Script_item_flags.java 11247 2005-01-11 23:22:22Z jz $

package sos.hostware;


class Script_item_flags
{
    private Script_item_flags( int i ) { value = i; }

    static final Script_item_flags none          = new Script_item_flags(          0 );
    static final Script_item_flags isvisible     = new Script_item_flags( 0x00000002 );
    static final Script_item_flags issource      = new Script_item_flags( 0x00000004 );
    static final Script_item_flags globalmembers = new Script_item_flags( 0x00000008 );
    static final Script_item_flags ispersistent  = new Script_item_flags( 0x00000010 );
    static final Script_item_flags nocode        = new Script_item_flags( 0x00000400 );
    static final Script_item_flags codeonly      = new Script_item_flags( 0x00000200 );

    public int value = 0;
}
