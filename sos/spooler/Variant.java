// $Id: Variant.java,v 1.1 2002/11/09 09:13:18 jz Exp $

package sos.spooler;


public class Variant
{
    public                      Variant                     ( int     value )                       { assign(value); }
    public                      Variant                     ( boolean value )                       { assign(value); }
    public                      Variant                     ( double  value )                       { assign(value); }
    public                      Variant                     ( String  value )                       { assign(value); }

    public native void          set_null                    ();
    public native boolean       is_null                     ();

    public native void          assign                      ( int     value );
    public native void          assign                      ( boolean value );
    public native void          assign                      ( double  value );
    public native void          assign                      ( String  value );

    public native int           as_int                      ();
    public native boolean       as_boolean                  ();
    public native int           as_double                   ();
    public native String        as_string                   ();
  //public native ..            as_date_time                ();
};
