/* sosdde.h                                             (c) SOS GmbH Berlin
                                                        Joacim Zschimmer
*/


#ifndef __SOSDDE_H
#define __SOSDDE_H

#if !defined( __SOSARRAY_H )
#   include "sosarray.h"
#endif

#if !defined __AREA_H
#   include "area.h"
#endif

namespace sos {
using namespace std;


#if defined __BORLANDC__        // Für DdeAddData, DdeGetData
    typedef void Dde_data;
#else
    typedef Byte Dde_data;
#endif

struct Sos_dde_locked : Xc
{
                                Sos_dde_locked          () : Xc( "SOS-DDE-005" ) {}
};

/*
struct Sos_lock
{
                                Sos_lock                () : _count( 0 ) {}

    void                        lock                    ()  { _count++; }
    void                        unlock                  ()  { _count--; }
    Bool                        locked                  () const { return _count != 0; }

  private:
    int                        _count;
};
*/

//-----------------------------------------------------------------Sosdde_format

struct Sosdde_format
{
    Sosdde_format( uint f ) : _format ( f ) {}

    uint _format;
};

ostream& operator<< ( ostream&, const Sosdde_format& );

ostream& operator<< ( ostream&, HDDEDATA );

//-------------------------------------------------------------------------------------------------

// DDE-Datentypen wie in ddeml.h definieren:

SOS_DECLARE_MSWIN_HANDLE32( HCONV    );
SOS_DECLARE_MSWIN_HANDLE32( HSZ      );
SOS_DECLARE_MSWIN_HANDLE32( HDDEDATA );
//bc5 typedef                     CONVCONTEXT;

//typedef Sos_string;

struct SOS_CLASS Sos_dde
{
    struct Sos_dde_error : Xc       // Fehlermeldungen von Sos_dde_server etc.
    {
        /* SOS-DDE-001  Ein zweiter DDE-Server kann nicht gestartet werden.
           SOS-DDE-002  DDE-Funktion nicht implementiert (DDE_FNOTPROCESSED)
        */
                                Sos_dde_error           ( int code )
                                                        : Xc( Msg_code( "SOS-DDE-", code, 3 ) ) {}
    };


    struct Dde_error : Xc           // Fehlermeldungen von DDE
    {
                                Dde_error               ( int code, const char* function_name );
                                Dde_error               ( const char* function_name );
        int                     dde_code                () const  { return numeric_suffix(); }
    };


    struct SOS_CLASS Ddeml_instance;
    struct SOS_CLASS Server;
    struct SOS_CLASS Conversation;

    struct Hsz
    {
                                Hsz                     ();
                                Hsz                     ( Ddeml_instance*, HSZ );

        void                    assign                  ( Ddeml_instance*, const char* );
        void                    del                     ();

        Bool                    operator ==             ( const Hsz& ) const;
                                operator HSZ            () const;
                                operator Sos_string     () const;
      //const char*             c_str                   () const;
        uint                    length                  () const;
        Bool                    empty                   () const;

      protected:
        uint4                  _ddeml_instance;
        HSZ                    _handle;
    };


    struct String_handle : Hsz
    {
                                String_handle           ();
                                String_handle           ( const Hsz& );
                                String_handle           ( Ddeml_instance*, const char* );
                                String_handle           ( Ddeml_instance*, const Sos_string& );
        virtual                ~String_handle           ();

      private:
        Bool                   _destroy;
    };


    struct Ddeml_instance : Sos_self_deleting
    {
                                Ddeml_instance          ();
        virtual                ~Ddeml_instance          ();

        HDDEDATA                event                   ( uint wType, uint wFmt, HCONV hConv,
                                                          HSZ topic, HSZ name,
                                                          HDDEDATA,
                                                          uint4 dwData1, uint4 dwData2 );

        HDDEDATA                event                   ( UINT wType, UINT wFmt, HCONV hConv,
                                                          const Hsz& hsz1, const Hsz& hsz2,
                                                          HDDEDATA hData,
                                                          DWORD dwData1, DWORD dwData2 );

                                operator uint4          ();
        void                    add                     ( Conversation* );
        void                    remove                  ( HCONV );

        DECLARE_PUBLIC_MEMBER( Server*, server_ptr )    // Zunächst nur ein Server
        DECLARE_PUBLIC_MEMBER( ostream*, log )

        Conversation*           conv_ptr                ( const Hsz& conv_name_handle );
        Conversation*&          conv_ptr                ( HCONV );
        int                     index                   ( HCONV );

        //Sos_lock               _lock;
        int                    _locked;

        uint                   _format_sos_binary;

      private:
        void                    init                    ();

        uint4                  _ddeml_instance;
        Sos_simple_array< Conversation* >   _conv_ptr_array;
    };


    struct SOS_CLASS Server
    {
                                Server                  ( const char* name );
        virtual                ~Server                  ();

        Ddeml_instance*         ddeml_instance_ptr      () const;

      //void                    add                     ( Conversation* );

        virtual Conversation*   xtyp_connect            ( const Hsz& topic, const CONVCONTEXT*,
                                                          Bool same_instance ) = 0;
        virtual void            error_event             ( uint2 code );
      //virtual void            monitor_event
      //virtual void            register_event
      //virtual void            unregister_event
      //virtual void            wild_connect_event
        HDDEDATA                event                   ( uint type, uint format, HCONV,
                                                          const Hsz& topic, const Hsz& item,
                                                          HDDEDATA,
                                                          uint4 dwData1, uint4 dwData2 );

      //Ddeml_instance*         ddeml_instance_ptr      () const { return _ddeml_instance_ptr; }

      protected:
        static void             xtyp_connect           ( HSZ service, HSZ topic,
                                                         const CONVCONTEXT*, Bool same_instance );

      private:
                                Server                  ( const Server& );  // Gesperrt

        Ddeml_instance*        _ddeml_instance_ptr;
        String_handle          _service_name_handle;

      //Sos_simple_array< Sos_dde::Conversation* >   _conv_ptr_array;
    };

    struct SOS_CLASS Conversation
    {
                                Conversation            ();
                                Conversation            ( Server*, const char* topic );
                                Conversation            ( Server*, const Hsz& topic );
        virtual                ~Conversation            ();

        HDDEDATA                event( UINT type, UINT format, HCONV conv_handle,
                                       const Hsz& topic_handle, const Hsz& item_handle, HDDEDATA data_handle,
                                       DWORD data_1, DWORD data_2 );

        virtual void            xtyp_connect_confirm    ();
        virtual void            xtyp_execute            ( HDDEDATA command, uint format );
        virtual void            xtyp_request            ( Const_area_handle*, const Hsz& item, uint format );
        virtual void            xtyp_poke               ( const Hsz& item, HDDEDATA data, uint format );
        virtual void            xtyp_advstart           ( const Hsz& item, uint format );
        virtual void            xtyp_advstop            ( const Hsz& item );
        virtual void            xtyp_advreq             ( Const_area_handle*, const Hsz& item, uint format, uint );
        virtual void            xtyp_disconnect         () {}
        virtual void            xtyp_advdata            ( const Hsz& item, HDDEDATA data, uint format );
        virtual void            xtyp_xact_complete      ( const Hsz& item, HDDEDATA data, uint format, int4 id );

        virtual Bool            ignore_error            ()     { return false; }
        virtual Sos_string      error_value             ()     { return Sos_string(); }

        CONVCONTEXT             context                 ();
        Server*                 server_ptr              () const;

      protected:
        HCONV                  _handle;

        friend struct           Ddeml_instance;
        void                    connect_confirm_event   ( HCONV );

        Server*                _server_ptr;             // 0: Client
        String_handle          _topic_handle;
        /*const*/ Xc*          _xc;
    };

    static Ddeml_instance*      ddeml_instance_ptr      ();

  private:
/*
  #if !defined( SYSTEM_WINDLL )
    static Ddeml_instance      _ddeml_instance;
   #else
    #error Sos_dde kann nicht in einer DLL verwendet werden!
    // Die statische Variable muß in den Applikationen gehalten werden.
    // Ein SOS-Struct, das die statischen Daten für eine DLL in der Applikation hält?
  #endif
*/
};

//typedef Sos_dde::Sos_dde_error  Sos_dde_error;
//typedef Sos_dde::Dde_error      Dde_error;

//void throw_dde_error( const char* dde_function_name, const char* ins1, const char* ins2 = 0 );
//void throw_dde_error( const char* dde_function_name );
void throw_dde_error( int code, const char* dde_function_name );
void throw_dde_error( int code, const char* dde_function_name, const char* ins1, const char* ins2 = 0 );
//void throw_sos_dde_error( int code );

//==========================================================================================inlines

//------------------------------------------------------------------------------------------------

inline Sos_dde::Hsz::Hsz()
 :  _ddeml_instance ( 0 ),
    _handle         ( 0 )
{
}

//------------------------------------------------------------------------------------------------

inline Bool Sos_dde::Hsz::empty() const
{
    return _handle == 0;
}

//------------------------------------------------------------------------------------------------

inline Sos_dde::Hsz::operator HSZ() const
{
    return _handle;
}
//------------------------------------------------------------------------------------------------

inline Sos_dde::String_handle::String_handle()
 :  _destroy ( false )
{
}

//------------------------------------------------------------------------------------------------

inline Sos_dde::Hsz::Hsz( Ddeml_instance* dde_server_ptr, HSZ hsz )
 :  _ddeml_instance ( *ddeml_instance_ptr() ),
    _handle         ( hsz )
{
}

//------------------------------------------------------------------------------------------------

inline Bool Sos_dde::Hsz::operator== ( const Sos_dde::Hsz& hsz2 ) const
{
    return _ddeml_instance == hsz2._ddeml_instance
           && _handle == hsz2._handle;
}

//------------------------------------------------------------------------------------------------

inline Sos_dde::String_handle::String_handle( Ddeml_instance* ddeml_instance_ptr,
                                              const char* string )
 :  _destroy ( true )
{
    assign( ddeml_instance_ptr, string );
}

//------------------------------------------------------------------------------------------------

inline Sos_dde::String_handle::String_handle( const Hsz& hsz )
 :  Hsz      ( hsz ),
    _destroy ( false )
{
}

//------------------------------------------------------------------------------------------------

inline Sos_dde::Ddeml_instance::Ddeml_instance()
:
    _ddeml_instance ( 0 ),
    _server_ptr     ( 0 ),
    _log            ( 0 ),
    _locked         ( 0 )
{
        _conv_ptr_array.obj_const_name( "Ddeml_instance::_conv_ptr_array" );
}

//------------------------------------------------------------------------------------------------

inline Sos_dde::Ddeml_instance::operator uint4()
{
    if( !_ddeml_instance ) {
        init();                               // DDE automatisch initiieren
    }
    return _ddeml_instance;
}

//------------------------------------------------------------------------------------------------
/*
inline void Sos_dde::Server::add( Conversation* conv_ptr )
{
    _conv_ptr_array.add( conv_ptr );
}
*/
//------------------------------------------------------------------Sos_dde::Server::ddeml_instance

inline Sos_dde::Ddeml_instance* Sos_dde::Server::ddeml_instance_ptr() const
{
    return _ddeml_instance_ptr;
}


} //namespace sos

#endif
