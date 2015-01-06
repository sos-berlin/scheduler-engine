
template<class TYPE>
void check_new( void* p ) ;


struct Sossql_status_windows
{
    void                        text                    ( const Sos_string& );
    Bool                        cancel_pressed          ()  { return _cancel_pressed; }

  private:
    static Sos_ptr<Record_type> make_type               ( Dynamic_dialog* = 0 );
    void                        cancel                  ()  { _cancel_pressed = true; }

    Fill_zero                  _zero_;
    Dynamic_dialog*            _dialog_ptr;
    Sos_string                 _text;
};

Sos_ptr<Record_type> Sossql_status_window::make_type( Dynamic_dialog* d )
{
BEGIN_RES_FIELDS
    SOSCTRL_ADD_FIELD ( text, 0 );
    SOSCTRL_ADD_METHOD( Sossql_status_window, cancel, 0 );
    SOSCTRL_ADD_FIXED( bild );   // res_dlg_bild
END_RES_FIELDS
}


Sossql_status_window::Sossql_status_window( Window* parent, const Sos_res_id& res_id )
{
    _dialog_ptr = check_new( new Dynamic_dialog( parent, res_id, SFT_MODELESS ) );
    make_type( _dialog_ptr );
    _dialog_ptr->record_ptr( &AREA( *this ) );
}


void Sossql_status_window::text( const Sos_string& )
{
    _text = text;
    _dialog_ptr->object_auto_load();
}



void main()
{
    Sossql_status_window w;

    w.text( "eins" );
