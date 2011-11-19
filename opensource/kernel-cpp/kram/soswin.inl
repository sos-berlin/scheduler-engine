// soswin.inl                                           (c) SOS GmbH Berlin
//                                                          Joacim Zschimmer

inline                          Key_code::Key_code     ( Value v, char c ) :  _value( v ), _char( c ) {}
inline                          Key_code::Key_code     () :  _value( 0 ), _char( 0 ) {}
inline Key_code::Key            Key_code::key          () const        { return Key( _value & _key_mask ); }
inline Key_code::Modifier       Key_code::modifier     () const        { return Modifier( _value & _modifier_mask );  }
inline Bool                     Key_code::shift_pressed() const        { return modifier() & shift; }
inline Bool                     Key_code::cntrl_pressed() const        { return modifier() & cntrl; }
inline Bool                     Key_code::alt_pressed  () const        { return modifier() & alt;   }
inline Key_code::Value          Key_code::value        () const        { return _value; }
inline void                     Key_code::chr          ( char c )      { _char = c;     }
inline char                     Key_code::chr          () const        { return _char;  }

inline Key_code                 Key_event::key_code    () const        { return _key_code; }     

inline void Abs_sos_window::handle_key_event( const Key_event& key_event )  { _handle_key_event( key_event ); }

inline Abs_sos_application::Abs_sos_application()
:
    _rescheduled ( 0 ),
    _terminate   ( false )
{
}

inline int      Abs_sos_application::main                   ( int argc, char* argv[] ) { _main( argc, argv ); return 0; }
inline void     Abs_sos_application::execute                ()                         { _execute();          }
inline int      Abs_sos_application::rescheduled            () const                   { return _rescheduled; }
inline Bool     Abs_sos_application::terminate              () const                   { return _terminate;   }
inline void     Abs_sos_application::busy                   ( Bool b )                 { _busy( b );          }
inline void     Abs_sos_application::add_open_event_count   ( int i )                  { _open_event_count += i ; }
inline int      Abs_sos_application::open_event_count       () const                   { return _open_event_count; } 


