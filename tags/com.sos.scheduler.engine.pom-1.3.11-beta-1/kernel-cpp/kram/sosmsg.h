// sosmsg.h                                     (c) SOS GmbH Berlin


#ifndef __SOSMSG_H
#define __SOSMSG_H

#if !defined SOSSTRNG_H
#   include "../kram/sosstrng.h"
#endif

namespace sos
{

//#if defined JZ_TEST  &&  !defined NDEBUG
#   define LOG_MSG
//#endif

// nach sos.h
/*

Botschaften-Protokoll:

BOTSCHAFTEN FÜR ANFORDERUNGEN:


Exceptions:
Löst eine Bearbeitungsroutine für eine Anforderungsbotschaft eine Exception Xc aus, wird diese
von send() in eine Error_msg( Xc ) konvertiert. Andere Exceptions werden mit Verlust der
Fehlerinformation ebenfalls in Error_msg konvertiert. Die Error_msg wird an den Absender
zurückgeschickt.

Die Bearbeitungsroutine darf eine Exception nur auslösen, wenn sie auf die Anforderung noch nicht
selbst geantwortet hat. Die Anforderung soll dann auch keine Seiteneffekte haben.


Create_msg( const char* name, ...?... )
    legt ein Objekt an

Antwort:
    Object_ref_msg oder Error_msg


Run_msg
    Startet ein Objekt. Das Objekt beginnt Data_msg nach msg_output_ptr() zu senden.

Antwort:
    Ack_msg (nicht mehr Finish_msg), wenn das Objekt fertig ist (also nach den Data_msg).
    Error_msg bei einem Fehler.


Data_msg( Const_area ):
    sendet Daten. Wird je nach Zielobjekt als Datensatz oder als Ausschnitt eines Streams
    interpretiert.

    Vielleicht wird noch optional die Satzposition mit übertragen werden.

Antwort:
    Ack_msg oder Error_msg.


End_msg:
    "EOF", wird nach Data_msg gesendet, wenn die Quelle versiegt.

Antwort:
    Ack_msg oder Error_msg.


Spec_msg( Object_type?, int code, Const_area parameter )
    Besondere Botschaft, die vom Zielobjekt interpretiert wird.
    Ein Art Object_type soll die richtige Typisierung sicherstellen.
    Objekttyp-Verwaltung? Klassenname -> Klassen-Handle.

Antwort:
    Ack_msg, Data_reply_msg, Object_ref_msg oder Error_msg.



BOTSCHAFTEN FÜR ANTWORTEN:

Ack_msg:
    Bestätigung.


Data_reply_msg( Const_area ):
    Wird je nach Zielobjekt als Datensatz oder als Ausschnitt eines Streams interpretiert.

    Vielleicht wird noch optional die Satzposition mit übertragen werden.


Object_ref_msg( Sos_object* ):
    Liefert den Zeiger auf ein neu angelegtes Objekt. Das Objekt wird Eigentum des Empfängers,
    welcher es am Ende schließen muß.


Error_msg( Xc ):
    asynchrones throw.


//AUSSERDEM:

//Async_error_msg( Xc ):
//    Fehlermeldung, die ein Server-Objekt unabhängig von einer Anforderung verschicken kann,
//    z.B. Verbindungsverlust.
*/

//-----------------------------------------------------------------------------------------forwards

//class ostream;
//class istream;
struct Sos_object;

//-------------------------------------------------------------------------------------Sos_msg_type

enum Sos_msg_type { sos_msg_type_dummy = 0x7FFF };

//---------------------------------------------------------------------------------Request_msg_type

enum Request_msg_type
{
    msg_create            = 0x01, // An eine Objekt-Fabrik, Antwort ist msg_object_ref
    msg_open              = 0x01,
    msg_destroy           = 0x02, // Objekt schließen (destruieren) == msg_end/msg_finish?
    msg_run               = 0x03, // Objekt starten, Objekt soll aktiv Daten von sich geben. Evtl. mit einer Art Dateiname.
    msg_end               = 0x04, // (Vorläufig?) letzte Meldung von der Quelle (EOF) (Close?),
    msg_data              = 0x05, // Datensatz (put, get)
    msg_spec              = 0x06, // Objekt-spezifische Botschaft

    msg_prepare_to_commit = 0x07, // Anschließend msg_commit oder msg_roll_back
    msg_commit            = 0x08, // Transaktion abschließen, wird mit msg_ack bestätigt
    msg_roll_back         = 0x09, // Transaktion zurückrollen, wird mit msg_ack bestätigt (vielleicht nicht nötig?)

    msg_seek              = 0x0A,
    msg_cancel            = 0x0B, // Alle Anforderungen abbrechen, alle Anforderungen außer cancel beantworten

    // Dateioperationen:
    msg_get               = 0x0101, // sequentiell lesen, Antwort ist Data_reply_msg
    msg_get_direct        = 0x0102, // direktes Lesen
    msg_store             = 0x0103  // direktes Schreiben
};

inline Bool is_request( Sos_msg_type t ) { return ( (int)t & 0x8000 ) == 0; }

//-----------------------------------------------------------------------------------Reply_msg_type

enum Reply_msg_type
{
    msg_ack           = 0x8001,  // Empfangsquittung, bereit für nächsten Datensatz
    msg_error         = 0x8002,  // Fehler, anstelle einer anderen Antwort
    msg_object_ref    = 0x8003,  // Objekt-Referenz
    msg_data_reply    = 0x8004
};

inline Bool is_reply( Sos_msg_type t ) { return ( (int)t & 0x8000 ) != 0; }


void                            send                    ( Sos_msg* );

//------------------------------------------------------------------------------------------Sos_msg

struct Sos_msg : Sos_self_deleting
{
    virtual                    ~Sos_msg                 ();

    DECLARE_PUBLIC_MEMBER( Sos_msg_type , type   )
  //DECLARE_PUBLIC_MEMBER( Sos_object*  , dest_ptr   )
    Sos_object*                 dest_ptr                () const                    { return _dest_ptr; }
    void                        dest_ptr                ( Sos_object* o )           { _dest_ptr = o; }
    DECLARE_PUBLIC_MEMBER( Sos_object*  , source_ptr )
    DECLARE_PUBLIC_MEMBER( uint4        , clock  )

    Sos_object*                 dest_object_ptr         () const                    { return dest_ptr(); }
    Bool                        is_request              () const                    { return ::sos::is_request( type() ); }
    Bool                        is_reply                () const                    { return ::sos::is_reply( type() ); }
  //friend inline ostream&      operator <<             ( ostream&, const Sos_msg& );

  //void*                       operator new            ( size_t );
  //void                        operator delete         ( void*, size_t );
    Sos_self_deleting*         _obj_copy                () const = 0;

  protected:
                                Sos_msg                 ( Sos_msg_type,
                                                          Sos_object* dest, Sos_object* src );

    virtual void               _obj_print               ( std::ostream* ) const;

  private:
    friend void                 send                    ( Sos_msg* );               // request() und reply() benutzen!

    Sos_object_ptr             _dest_ptr;
};

//--------------------------------------------------------------------------------------Request_msg

struct Request_msg : Sos_msg
{
    Request_msg_type            type                    () const        { return (Request_msg_type) Sos_msg::type(); }

    friend inline void          request                 ( Request_msg* );
    friend inline void          post_request            ( const Request_msg* );

  protected:
    inline                      Request_msg             ( Request_msg_type,
                                                          Sos_object* dest, Sos_object* src );
};

//----------------------------------------------------------------------------------------Reply_msg

struct Reply_msg : Sos_msg
{
    Reply_msg_type              type                    () const        { return (Reply_msg_type) Sos_msg::type(); }

    friend inline void          reply                   ( const Reply_msg& );
    friend inline void          reply                   ( Reply_msg* );
    DEFINE_OBJ_COPY( Reply_msg )

  protected:
                                Reply_msg               ( Reply_msg_type,
                                                          Sos_object* dest, Sos_object* src );
};

//-----------------------------------------------------------------------------------Create_msg

struct Create_msg : Request_msg
{
                                Create_msg              ( Sos_object*, Sos_object*, const Sos_string& name, Sos_object* owner = 0 );

    const char*                 name                    () const        { return c_str( _name ); }
    DEFINE_OBJ_COPY( Create_msg )

  protected:
    virtual void               _obj_print               ( std::ostream* ) const;
  //virtual Sos_msg*            new_copy                () const;

  public:
    Sos_object*                _owner;
    Sos_string                 _name;                   // ERFORDERT sosstrng.h ...
  //char                       _name [ 200 ];
};

typedef Create_msg T_create_msg;

typedef Create_msg Open_msg;
typedef Open_msg T_open_msg;

//------------------------------------------------------------------------------------------Run_msg

struct Run_msg : Request_msg
{
                                Run_msg                 ( Sos_object*, Sos_object* );
    DEFINE_OBJ_COPY( Run_msg )

  protected:
  //virtual Sos_msg*            new_copy                () const;
    virtual void               _obj_print               ( std::ostream* ) const;
};

typedef Run_msg T_run_msg;

//-----------------------------------------------------------------------------------------Data_msg

struct Data_msg : Request_msg
{
                                Data_msg                ( Sos_object*, Sos_object*,
                                                          const Const_area/*_handle*/&,
                                                          uint4 seek_position = (uint4)-1 );

                                Data_msg                ( Sos_object*, Sos_object*,
                                                          const Const_area/*_handle*/&,
                                                          const Const_area_handle& key );

    const Const_area&           data                    () const            { return _data; }
    const Const_area_handle&    key                     ();

    Const_area/*_handle*/      _data;
    uint4                      _seek_position;
    Const_area_handle          _key;
    DEFINE_OBJ_COPY( Data_msg )

  protected:
  //virtual Sos_msg*            new_copy                () const;
    virtual void               _obj_print               ( std::ostream* ) const;
};

typedef Data_msg T_data_msg;

//------------------------------------------------------------------------------------------End_msg

struct End_msg : Request_msg
{
                                End_msg                 ( Sos_object*, Sos_object* );
    DEFINE_OBJ_COPY( End_msg )

  protected:
  //virtual Sos_msg*            new_copy                () const;
    virtual void               _obj_print               ( std::ostream* ) const;
};

typedef End_msg T_end_msg;

//-----------------------------------------------------------------------------------------Spec_msg

struct Spec_msg : Request_msg
{
                                Spec_msg                ( Sos_object*, Sos_object*,
                                                          int spec_code, const Const_area& );

    int                         spec_code               () const            { return _spec_code; }
    const Const_area&           data                    () const            { return _data; }
  //virtual Sos_msg*            new_copy                () const;
    virtual void               _obj_print               ( std::ostream* ) const;
    DEFINE_OBJ_COPY( Spec_msg )

  private:
    int                        _spec_code;
    Const_area                 _data;
};

typedef Spec_msg T_spec_msg;

//-----------------------------------------------------------------------------------Cancel_msg

struct Cancel_msg : Request_msg
{
                                Cancel_msg              ( Sos_object*, Sos_object* );
    DEFINE_OBJ_COPY( Cancel_msg )

  protected:
  //virtual Sos_msg*            new_copy                () const;
    virtual void               _obj_print               ( std::ostream* ) const;
};

typedef Cancel_msg T_cancel_msg;

//-------------------------------------------------------------------------------Data_reply_msg

struct Data_reply_msg : Reply_msg
{
                                Data_reply_msg          ( Sos_object*, Sos_object*,
                                                          const Const_area_handle&,
                                                          int4 seek_position = -1 );

                                Data_reply_msg          ( Sos_object*, Sos_object*,
                                                          const Const_area_handle& record,
                                                          const Const_area_handle& key );

    const Const_area_handle&    data                    () const            { return _data; }
    int4                        seek_pos                () const            { return _seek_pos;  }
    const Const_area_handle&    key                     ();
    DEFINE_OBJ_COPY( Data_reply_msg )

  protected:
  //virtual Sos_msg*            new_copy                () const;
    virtual void               _obj_print               ( std::ostream* ) const;

  public:
    Const_area_handle          _data;
    int4                       _seek_pos;               // -1: keine seek_position
    Const_area_handle          _key;                    // length() == 0: kein key
};

typedef Data_reply_msg T_data_reply_msg;

//--------------------------------------------------------------------------------Data_with_key_msg
/*
struct Data_with_key_msg : Data_msg {  };
*/
//------------------------------------------------------------------------------------------Ack_msg

struct Ack_msg : Reply_msg
{
#   if defined __SOSFIELD_Hxxx
        struct Ext : Sos_msg::Ext  {};
#   endif

                                Ack_msg                 ( Sos_object*, Sos_object* );

    DEFINE_OBJ_COPY( Ack_msg )

  protected:
  //virtual Sos_msg*            new_copy                () const;
    virtual void               _obj_print               ( std::ostream* ) const;
};

typedef Ack_msg T_ack_msg;

//-----------------------------------------------------------------------------------Object_ref_msg

struct Object_ref_msg : Reply_msg
{
                                Object_ref_msg          ( Sos_object*, Sos_object*,
                                                          Sos_object* );

    Sos_object_ptr              object_ptr              () const            { return _object_ptr; }
  //virtual Sos_msg*            new_copy                () const;
    virtual void               _obj_print               ( std::ostream* ) const;

    DEFINE_OBJ_COPY( Object_ref_msg )

  private:
    Sos_object_ptr             _object_ptr;
};

typedef Object_ref_msg T_object_ref_msg;

//--------------------------------------------------------------------------------------Destroy_msg

struct Destroy_msg : Request_msg
{
                                Destroy_msg            ( Sos_object*, Sos_object*, Close_mode = close_normal );

    Close_mode                  mode                    () const        { return _mode; }
    DEFINE_OBJ_COPY( Destroy_msg )

  protected:
    virtual void               _obj_print               ( std::ostream* ) const;
  //virtual Sos_msg*            new_copy                () const;

  private:
    Close_mode                 _mode;
};

typedef Destroy_msg T_destroy_msg;

//----------------------------------------------------------------------------Prepare_to_commit_msg

struct Prepare_to_commit_msg : Request_msg
{
                                Prepare_to_commit_msg   ( Sos_object*, Sos_object* );
    DEFINE_OBJ_COPY( Prepare_to_commit_msg )

  protected:
  //virtual Sos_msg*            new_copy                () const;
  //virtual void               _obj_print               ( ostream* ) const;
};

typedef Prepare_to_commit_msg T_prepare_to_commit_msg;

//---------------------------------------------------------------------------------------Commit_msg

struct Commit_msg : Request_msg
{
                                Commit_msg              ( Sos_object*, Sos_object* );
  //virtual Sos_msg*            new_copy                () const;
    DEFINE_OBJ_COPY( Commit_msg )

  protected:
  //virtual void               _obj_print               ( ostream* ) const;
};

typedef Commit_msg T_commit_msg;

//------------------------------------------------------------------------------------Roll_back_msg

struct Roll_back_msg : Request_msg
{
                                Roll_back_msg           ( Sos_object*, Sos_object* );
  //virtual Sos_msg*            new_copy                () const;
    DEFINE_OBJ_COPY( Roll_back_msg )

  protected:
  //virtual void               _obj_print               ( ostream* ) const;
};

typedef Roll_back_msg T_roll_back_msg;

//--------------------------------------------------------------------------------------Get_msg

struct Get_msg : Request_msg
{
                                Get_msg                 ( Sos_object*, Sos_object*, uint length = 0 );
  //virtual Sos_msg*            new_copy                () const;

  //Area*                       area_ptr                ()                      { return _area_ptr; }
    uint                        length                  () const                { return _length; }

    uint                       _length;
    DEFINE_OBJ_COPY( Get_msg )

  protected:
                                Get_msg                 ( Request_msg_type, Sos_object*, Sos_object* );
    virtual void               _obj_print               ( std::ostream* ) const;

  private:
  //Area*                      _area_ptr;
};

typedef Get_msg T_get_msg;

//--------------------------------------------------------------------------------------Get_msg

struct Seek_msg : Request_msg
{
                                Seek_msg                ( Sos_object* dest, Sos_object* src, uint4 pos ) : Request_msg( msg_seek, dest, src ), _pos ( pos ) {}

    //virtual Sos_msg*            new_copy                () const    { return new Seek_msg( *this ); }
    DEFINE_OBJ_COPY( Seek_msg )

    uint4                      _pos;
};

typedef Seek_msg T_seek_msg;

//------------------------------------------------------------------------------------Error_msg

struct Error_msg : Reply_msg
{
                                Error_msg               ( Sos_object*, Sos_object*, const Xc& );
    const Xc&                   error                   () const                        { return _error; }
  //virtual Sos_msg*            new_copy                () const;
    DEFINE_OBJ_COPY( Error_msg )

  protected:
                                Error_msg               ( Reply_msg_type, Sos_object*, Sos_object*, const Xc& );
    virtual void               _obj_print               ( std::ostream* ) const;

  private:
    Xc                         _error;
};

typedef Error_msg T_error_msg;


void dispatch_waiting_msg();        // Ohne Betriebsystem-Ereignisse (Windows-Botschaften)
void sos_msg_dispatcher();
void sos_msg_dispatcher( int count );
void sos_msg_wait_for_msg_to( const Sos_object* );
inline void wait_for_msg_to( const Sos_object* o )      { sos_msg_wait_for_msg_to( o ); }
void _post( const Sos_msg& );
void _post( Sos_msg* );
void reply_ack_msg( Sos_object* dst, Sos_object* src );
void reply_data_reply_msg( Sos_object* dst, Sos_object* src, const Const_area_handle& area );
void reply_error_msg( Sos_object* dst, Sos_object* src, const Xc& x );
void reply_error_msg( Sos_object* dst, Sos_object* src, const char* error_code );

//==========================================================================================inlines

//---------------------------------------------------------------------------------Sos_msg::Sos_msg

inline Sos_msg::Sos_msg( Sos_msg_type type, Sos_object* dest, Sos_object* source )
:
    _type       ( type   ),
    _dest_ptr   ( dest   ),
    _source_ptr ( source ),
    _clock      ( 0          )
{
}

//--------------------------------------------------------------------------operator << ( Sos_msg )
/*
inline ostream& operator<< ( ostream& s, const Sos_msg& m )
{
    m._obj_print( &s );
    return s;
}
*/
//-------------------------------------------------------------------------Request_msg::Request_msg

inline Request_msg::Request_msg( Request_msg_type type, Sos_object* dest, Sos_object* source )
:
    Sos_msg( (Sos_msg_type)type, dest, source )
{
}

//-----------------------------------------------------------------------------Reply_msg::Reply_msg

inline Reply_msg::Reply_msg( Reply_msg_type type, Sos_object* dest, Sos_object* source )
:
    Sos_msg( (Sos_msg_type)type, dest, source )
{
}

//-----------------------------------------------------------------------Create_msg::Create_msg

inline Create_msg::Create_msg( Sos_object* dest, Sos_object* source, const Sos_string& name, Sos_object* owner  )
:
    Request_msg ( msg_create, dest, source ),
    _owner ( owner )
{
    _name = name;
}

//---------------------------------------------------------------------------------Run_msg::Run_msg

inline Run_msg::Run_msg( Sos_object* dest, Sos_object* source )
:
    Request_msg ( msg_run, dest, source )
{
}

//-------------------------------------------------------------------------------Data_msg::Data_msg

inline Data_msg::Data_msg( Sos_object* dest, Sos_object* source, const Const_area& area,
                           uint4 seek_position )
:
    Request_msg( msg_data, dest, source ),
    _data      ( area ),
    _seek_position ( seek_position )
{
}

//-------------------------------------------------------------------------------Data_msg::Data_msg

inline Data_msg::Data_msg( Sos_object* dest, Sos_object* source, const Const_area& area,
                           const Const_area_handle& key )
:
    Request_msg( msg_data, dest, source ),
    _data      ( area ),
    _seek_position ( (uint4)-1 ),
    _key       ( key )
{
}

//---------------------------------------------------------------------------------Ack_msg::Ack_msg

inline Ack_msg::Ack_msg( Sos_object* dest, Sos_object* source )
:
    Reply_msg ( msg_ack, dest, source )
{
}

//---------------------------------------------------------------------------------End_msg::End_msg

inline End_msg::End_msg( Sos_object* dest, Sos_object* source )
:
    Request_msg ( msg_end, dest, source )
{
}

//-------------------------------------------------------------------------Destroy_msg::Destroy_msg

inline Destroy_msg::Destroy_msg( Sos_object* dest, Sos_object* source, Close_mode mode )
:
    Request_msg ( msg_destroy, dest, source ),
    _mode       ( mode )
{
}

//-------------------------------------------------------------------------------Spec_msg::Spec_msg

inline Spec_msg::Spec_msg( Sos_object* dest, Sos_object* source,
                           int spec_code, const Const_area& data )
:
    Request_msg ( msg_spec, dest, source ),
    _spec_code  ( spec_code ),
    _data       ( data      )
{
}

//---------------------------------------------------------------------------------Get_msg::Get_msg

inline Get_msg::Get_msg( Sos_object* dest, Sos_object* src, uint length )
:
    Request_msg  ( msg_get, dest, src ),
    _length      ( length    )
{
}

//---------------------------------------------------------------------------------Get_msg::Get_msg

inline Get_msg::Get_msg( Request_msg_type type, Sos_object* dest, Sos_object* src )
:
    Request_msg  ( type, dest, src )
{
}

//-------------------------------------------------------------------Data_reply_msg::Data_reply_msg

inline Data_reply_msg::Data_reply_msg( Sos_object* dest, Sos_object* src,
                                       const Const_area_handle& h, int4 seek_pos )
:
    Reply_msg ( msg_data_reply, dest, src ),
    _data     ( h         ),
    _seek_pos ( seek_pos )
{
}

//-------------------------------------------------------------------Data_reply_msg::Data_reply_msg

inline Data_reply_msg::Data_reply_msg( Sos_object* dest, Sos_object* src,
                                       const Const_area_handle& h, const Const_area_handle& key )
:
    Reply_msg ( msg_data_reply, dest, src ),
    _data     ( h         ),
    _seek_pos ( -1 ),
    _key      ( key )
{
}

//-------------------------------------------------------------------Object_ref_msg::Object_ref_msg

inline Object_ref_msg::Object_ref_msg( Sos_object* dest, Sos_object* src, Sos_object* object_ptr )
:
    Reply_msg   ( msg_object_ref, dest, src ),
    _object_ptr ( object_ptr )
{
}

//-----------------------------------------------------------------------------Error_msg::Error_msg

inline Error_msg::Error_msg( Sos_object* dest, Sos_object* src, const Xc& error )
:
    Reply_msg   ( msg_error, dest, src ),
    _error      ( error                )
{
}

//------------------------------------------------------------------------------------------request

inline void request( Request_msg* msg_ptr )
{
    msg_ptr->dest_ptr()->_request_msg( msg_ptr );
}

//------------------------------------------------------------------------------------------request
#if !defined LOG_MSG

#   define MSG_DEFINE_REQUEST( MSG_TYPE, METHOD )                                               \
                                                                                                 \
        inline void request( MSG_TYPE* m )                                                       \
        {                                                                                        \
            m->dest_ptr()->METHOD( m );                                                          \
        }

    DEFINE_DIRECT_SEND( Run_msg              , _obj_run_msg               )
    DEFINE_DIRECT_SEND( Data_msg             , _obj_data_msg              )
    DEFINE_DIRECT_SEND( End_msg              , _obj_end_msg               )
    DEFINE_DIRECT_SEND( Spec_msg             , _obj_spec_msg              )
    DEFINE_DIRECT_SEND( Create_msg           , _obj_create_msg            )
    DEFINE_DIRECT_SEND( Destroy_msg          , _obj_destroy_msg           )
    DEFINE_DIRECT_SEND( Prepare_to_commit_msg, _obj_prepare_to_commit_msg )
    DEFINE_DIRECT_SEND( Commit_msg           , _obj_commit_msg            )
    DEFINE_DIRECT_SEND( Roll_back_msg        , _obj_roll_back_msg         )

#   define MSG_DEFINE_REPLY( PROC, MSG_TYPE, METHOD )                                           \
                                                                                                 \
        inline void reply( const MSG_TYPE& m )                                                   \
        {                                                                                        \
            m->dest_ptr()->METHOD( m );                                                          \
        }

    DEFINE_DIRECT_SEND( Ack_msg              , _obj_ack_msg               )
    DEFINE_DIRECT_SEND( Object_ref_msg       , _obj_object_ref_msg        )
    DEFINE_DIRECT_SEND( Error_msg            , _obj_ack_msg               )

#   undef MSG_DEFINE_REQUEST
#   undef MSG_DEFINE_REPLY
#endif

inline void reply( const Reply_msg& m )
{
    if( m.source_ptr() ) {
        m.source_ptr()->_obj_request_semaphore++;      // s.a. Sos_object::_request_msg()
    }
    _post( m );
}

inline void reply( Reply_msg* m )
{
    if( m->source_ptr() ) {
        m->source_ptr()->_obj_request_semaphore++;      // s.a. Sos_object::_request_msg()
    }
    _post( m );
}

inline void post_request( const Request_msg& m )
{
    _post( m );
}

inline void post_request( Request_msg* m )
{
    _post( m );
}


} //namespace sos

#endif
