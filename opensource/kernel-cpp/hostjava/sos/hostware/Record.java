// $Id: Record.java 11346 2005-03-24 10:00:11Z jz $

package sos.hostware;

/**
 * Ein aus Feldern bestehender Datensatz.
 *
 * <p>Wird von {@link sos.hostware.File#get()} geliefert. Die C++-Resourcen sollten finally mit {@link #destruct()} freigegeben werden.</p>
 * <p>Die Feldnummern beginnen mit 0.</p>
 *
 * @author Joacim Zschimmer
 */

public final class Record  extends Idispatch    // (Dynobj)
{
    private                     Record                      ( long idispatch )                                  { super( idispatch ); }

  //public void                 add_field                   ( String name, Type type );
  //public void                 add_field                   ( String name, String type );
  
    /** Liefert die Anzahl der Felder im Datensatz. */
    public int                  field_count                 ()                              throws Exception    { return        int_com_call( "<obj_field_count"                        ); }
    
    /** Liefert den Namen des angegebenen Feldes. */
    public String               field_name                  ( int field_nr )                throws Exception    { return     string_com_call( "<obj_field_name"         , field_nr      ); }
    
    public void             set_write_empty_as_null         ( boolean b )                   throws Exception    {                   com_call( ">obj_write_empty_as_null", b             ); }
    
    /** Liefert die Nummer des angegebenen Feldes. */
    public int                  field_index                 ( String name )                 throws Exception    { return        int_com_call( "<obj_field_index"        , name          ); }
    
  //public Record               clone                       ();                                                                                         
  //public Variant              field                       ( int nr )                      throws Exception    { return     string_com_call( "<"                       , nr            ); }
  //public Variant              field                       ( String name )                 throws Exception    { return     string_com_call( "<"                       , name          ); }
  
    /** Liefert den Inhalt des Feldes als String. Ein Null-Wert wird als null geliefert. */
    public String               string                      ( int field_number )            throws Exception    { return     string_com_call( "<"                       , field_number  ); }
    
    /** Liefert den Inhalt des Feldes als String. Ein Null-Wert wird als null geliefert. */
    public String               string                      ( String field_name )           throws Exception    { return     string_com_call( "<"                       , field_name          ); }
    
  //public void             set_field                       ( int nr, String value )        throws Exception    {                   com_call( ">"                       , new Integer(nr), value   ); }
  //public void             set_field                       ( String name, String value )   throws Exception    {                   com_call( ">"                       , name, value   ); }
  //public String               xml                         ( String options )              throws Exception    { return     string_com_call( "<obj_xml"                , options       ); }
  
    /** Liefert den Datensatz als XML-Ausschnitt. */
    public String               xml                         ()                              throws Exception    { return     string_com_call( "<obj_xml"                                ); }
    
  //public void             set_xml                         ( String options, String xml )                      {                   com_call( ">obj_xml"                , xml           ); }
  //public Type                 type                        ();

    /** {@link #string(String)} soll Null-Werte als "" liefern. */
    public void                 read_null_as_empty          ()                              throws Exception    {                   com_call( ">obj_read_null_as_empty" , true          ); }
    

    
    /** Liefert true, wenn der Datensatz den Feldnamen kennt */
    public boolean field_exists( String name )
    {
        try
        {
            field_index( name );
            return true;
        }
        catch( Exception x )
        {
            return false;
        }
    }

};

