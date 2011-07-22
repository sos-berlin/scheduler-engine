// sosstat.h                                     © 1995 SOS GmbH Berlin

#ifndef __SOSSTAT_H
#define __SOSSTAT_H

/*
    Jede Task hat ein Exemplar von Sos_static. Es soll die task-spezifischen statics ersetzen.
    In einer WIN16-DLL sind die statics system-global.

    Instanzen von Sos_static werden mit Sos_object::operator new() angefordert, sodaß alle
    nicht initialisierten Elemente 0 sind.
*/

#if !defined __XCEPTION_H
#   include "xception.h"
#endif

#include "../zschimmer/mutex.h"

#if defined SYSTEM_WIN
    SOS_DECLARE_MSWIN_HANDLE( HINSTANCE )
    SOS_DECLARE_MSWIN_HANDLE( HTASK )

#   if defined SYSTEM_MICROSOFT
        typedef void* HANDLE;
#   endif

#   if defined SYSTEM_WINDLL  &&  defined SYSTEM_WIN16
        struct Sos_static_task_entry;
#   endif
#endif

namespace sos
{

//-------------------------------------------------------------------------------------------static

extern z::Mutex hostware_mutex;     // Allgemeine Semaphore für Hostole (und vielleicht andere, z.B. hostphp und C++-Hostware-Programme)

//-------------------------------------------------------------------------------------------------

struct Hostword_static;
struct Hostapi;
struct Sos_client;
struct Sos_msg_queue;
struct Sos_socket_manager;
struct Any_file;
struct Fle370;
struct Fs_common;
struct Sosdb_static;
struct Type_register;
struct Odbc_static;
struct Sosalloc_static;
struct Sossql_static;
struct Comfile_static;
struct Sos_licence;
struct Sos_function_register;
struct Sos_timer_manager;
struct Sosprof_static;
struct Ebo_static;
struct Cache_file_static;

//-----------------------------------------------------------------------------------Profile_source
// Für sosprof.cxx (gcc 3.3.2 erlaubt nicht die Vorwärtsdekaration mit enum Profile_source;.

enum Profile_source         // Gelesene Quellen des Dateinamens für sos.ini. Gibt auch die Priorität an. Niedrige Priorität kann höhere nicht überschreiben.
{
    source_default      = 0x01,     // "sos.ini"
    source_environment  = 0x02,     // SOS_INI
    source_program      = 0x04,     // -sos.ini=
    source_registry     = 0x08      // Windows-Registrierung
};

//-------------------------------------------------------------------------------------------------

namespace mail
{
    struct Mail_static;
};

//---------------------------------------------------------------------------------Hostware_version

struct Hostware_version
{
                                Hostware_version        ( const string& version )                   { *this = version; }
                                Hostware_version        ( int major, int minor, int count, int = 0 )    : _major(major), _minor(minor), _count(count) {}

    Hostware_version&           operator =              ( const string& version );
                                operator string         () const;

    bool                        operator <              ( const Hostware_version& v ) const         { return cmp( v ) <  0; }
    bool                        operator <=             ( const Hostware_version& v ) const         { return cmp( v ) <= 0; }
    bool                        operator ==             ( const Hostware_version& v ) const         { return cmp( v ) == 0; }
    bool                        operator !=             ( const Hostware_version& v ) const         { return cmp( v ) != 0; }
    bool                        operator >=             ( const Hostware_version& v ) const         { return cmp( v ) >= 0; }
    bool                        operator >              ( const Hostware_version& v ) const         { return cmp( v ) >  0; }

    int                         cmp                     ( const Hostware_version& ) const;


    int                        _major;
    int                        _minor;
    int                        _count;
};

//-------------------------------------------------------------------------------------------------

struct Sos_static_0
{
                                                        Sos_static_0();
                                                       ~Sos_static_0();

    void                                                init0();                                                       
    void                                                close0      ();

    Fill_zero                                          _zero_;
    Bool                                               _valid;
    zschimmer::Mutex                                   _log_lock;
    zschimmer::Log_context                             _log_context;
    zschimmer::Log_context*                            _log_context_ptr;            // NULL oder &_log_context
    ::std::ostream*                                    _log_ptr;                   // log.cxx
    string                                             _log_filename;
    string                                             _log_categories;

    //int                                                _log_indent_tls;            // log.cxx
#   if !defined SYSTEM_WIN
        int                                            _log_indent;                // log.cxx
#   endif

  //uint4                                              _object_count;
    bool                                               _dont_check_licence;         // Für hostole.dll
};


struct Sos_static : Sos_static_0
{
                                                        Sos_static();
                                                       ~Sos_static();

    void                                                add_ref     ()              { if( ++_ref_count == 1 )  init(); }
    void                                                remove_ref  ()              { if( --_ref_count == 0 )  close_current_task(); }
    void                                                init        ();
    void                                                close       ();

    void                                                use_version ( const string& version );
    string                                          get_use_version()                       { return _use_version; }
  //bool                                                since_version( int major, int minor, int count );
    bool                                                since_version( const string& version );
    bool                                                since_version( const Hostware_version& version );
    void                                                need_version ( const string& version );
    Hostware_version                                    version      ();
    bool                                                is_version_or_later( const string& );
                                                                                                                

    Fill_zero                                          _zero_;

#   if defined SYSTEM_WIN
#       if defined SYSTEM_WIN32        
            HANDLE                                     _htask;
#        else
            HTASK                                      _htask;
#       endif
        HINSTANCE                                      _hinstance;                 // von WinMain, nicht hInst der DLL
#   endif

    int                                                _index;
    int                                                _ref_count;
    int                                                _get_error_text_active;
    int                                                _log_active;
    Bool                                               _multiple_clients;
    enum Profile_source                                _profile_source;            // Woher die aktuelle Einstellung gelesen worden ist.
    enum Profile_source                                _profile_source_checked;    // Welche Einstellung bereits geprüft worden sind.
  //bool                                               _profile_registry_read;
    string                                             _profile;                   // Name der sos.ini oder leer
  //bool                                               _profile_locked;            // Wenn sos.ini in der Windows-Registrierung festgelegt ist, dann wird der Eintrag gesperrt
    Hostware_version                                   _use_version;
    int4                                               _reserve_int4 [ 20 ];

#ifdef USE_SOSALLOC
    Sosalloc_static*                                   _sosalloc;
#endif

    Sos_static_ptr<mail::Mail_static>                  _mail;
    Sos_static_ptr<Cache_file_static>                  _cache_file;
    Sos_static_ptr<Ebo_static>                         _ebo;                        // Eichenauer
    Sos_static_ptr<Sos_timer_manager>                  _timer_manager;
    Sos_static_ptr<Hostword_static>                    _hostword;                   // hostword.cxx (letter)
    Sos_static_ptr<Hostapi>                            _hostapi;
    Sos_static_ptr<Sos_client>                         _std_client;
    Sos_pointer                                        _init_parameters_ptr;       // init.cxx
    Sos_static_ptr<Type_register>                      _type_register;
    Sos_pointer                                        _file_type_common_head;
    Sos_pointer                                        _factory_agent;             // sosfact.cxx
    Sos_static_ptr<Sos_msg_queue>                      _msg_queue_ptr;             // sosmsg.cxx
    Sos_static_ptr<Any_file>                           _error_text_file;
    Sos_pointer                                        _ddeml_instance_ptr;        // sosdde.cxx
    Sos_pointer                                        _wbtrint2_ptr;              // wbtrint2.cxx
    Sos_pointer                                        _mswin_msg_window_manager_ptr;
    Sos_static_ptr<Sos_socket_manager>                 _socket_manager_ptr;        // sossock.cxx
    Sos_static_ptr<Fs_common>                          _fs_common;                 // fsfile.cxx
    Sos_static_ptr<Odbc_static>                        _odbcfile;
    Sos_static_ptr<Sosdb_static>                       _sosdb;
    Sos_static_ptr<Fle370>                             _fle370;
    Sos_static_ptr<Sossql_static>                      _sossql;
    Sos_static_ptr<Comfile_static>                     _comfile;
    Sos_static_ptr<Sos_licence>                        _licence;
    Sos_static_ptr<Sos_function_register>              _function_register;
    Sos_static_ptr<Sosprof_static>                     _sosprof;                   // sosprof.cxx (.ini-files)
    Sos_pointer                                        _reserve [ 35 ];

    static void                 close_current_task()    { if (_sos_static)  _sos_static->close(); }

  private:
    static Sos_static*             _sos_static;

    friend inline Sos_static*       sos_static_ptr();
    friend inline Sos_static*       sos_static_ptr_static();
};

//-------------------------------------------------------------------------------sos_static_ptr
Sos_static* sos_static_ptr_static();

inline Sos_static* sos_static_ptr()
{
    return !Sos_static::_sos_static? sos_static_ptr_static() : Sos_static::_sos_static;
}

//-------------------------------------------------------------------------------Has_static_ptr

struct SOS_CLASS Has_static_ptr   // Cache für sos_static_ptr()
{
# if defined SYSTEM_WINDLL  &&  defined SYSTEM_WIN16

    protected:
                                Has_static_ptr          ();
                                Has_static_ptr          ( Sos_static* s ) : _static_ptr( s ) {}
      Sos_static*               sos_static_ptr          () const  { return _static_ptr; }

    private:
      Sos_static* const        _static_ptr;

#  else

    protected:
      Sos_static*               sos_static_ptr          () const  { return ::sos::sos_static_ptr(); }

# endif
};

#if defined USE_OLD_EXCEPTIONS
    inline Exception* xc_ptr()
    {
        return &sos_static_ptr()->_xc;
    }
#endif

} //namespace sos

#endif
