#include <precomp.h>

//#define MODULE_NAME "sosfile"
#if 0

#include <sosfile.h>
#include <jzincl.h>
#include <anyfile.h>

#include <xception.h>

#include <init.h>

#ifdef _Windows
#  define EBCDIC_TO_ISO ebc2iso_german
#  define ISO_TO_EBCDIC iso2ebc_german
#else
#  define EBCDIC_TO_ISO ebc2iso
#  define ISO_TO_EBCDIC iso2ebc
#endif

#ifdef __DLL__
#  include <windows.h>
#  include <toolhelp.h>
#  define SOSDLL pascal _far _export
#else
#  include <stdlib.h> // wg. atexit()
#endif

#ifdef __DLL__
struct Task_entry {
  Task_entry( HTASK task );
  ~Task_entry();

  friend Task_entry;

  HTASK Task() { return _task; };
  void  store_exception() { 
    set_exception_name(); 
    set_error_code(); 
    if (_XC.raised()) discard_exception();
  };
  void  store_errno( int errno ) { _errno = errno; };

  const char* exception_name() { return _exception; };
  const char* error_code() { return _error_code; };
  const int errno() { return _errno; };

  static Task_entry* lookup( HTASK task, BOOL bCreate = FALSE );
  static void delete_all();

private:
  void set_exception_name() { strcpy(_exception,_XC.name()); };
  void set_error_code() { strcpy(_error_code,_XC.error_code()); };

  HTASK       _task;
  char        _task_name[8+1];    // DOS-Name
  char        _exception[20+1];   // letzte Exception
  char        _error_code[20+1];  // letzter Error_Code
  int         _errno;             // letzter Errno
  Task_entry* _next;
  Task_entry* _prev;              // nur wg. Loeschen!

  static Task_entry* _head;
};

extern const char* app_name();

Task_entry::Task_entry( HTASK task ) :
   _errno( 0 ),
   _task( task ),
   _next(_head),
   _prev(NULL)
{
   strcpy(_error_code,""),
   strcpy(_exception,""),
   strcpy(_task_name,app_name());
   _head = this;
};

Task_entry::~Task_entry()
{
    if ( _prev ) _prev->_next = _next;
    if ( _next ) _next->_prev = _prev;
};        

void Task_entry::delete_all() {
   if ( !_head ) return;
   for( Task_entry* p=_head->_next; p != NULL; ) {
      Task_entry* n = p->_next;
      delete p; p = NULL;
      p = n;
   };
   delete _head;
};

Task_entry* Task_entry::lookup( HTASK task, BOOL bCreate ) {
   for( Task_entry* p=_head; p != NULL; p = p->_next ) {
      if ( p->Task() == task ) return p;
   };
   if ( p == NULL && bCreate ) {
     p = new Task_entry( task );
       if ( !p ) raise( "NOMEMORY", "R101" );
   };
   return p;
exception_handler:
   return NULL;
};

Task_entry* Task_entry::_head;

#endif

const File_array_size = 100;
struct File_entry {
  Dynamic_any_file* _file;
  int               _is_nuc;
#ifdef __DLL__
  Task_entry*       _task_entry;
#endif
};


static File_entry File_array[File_array_size];

#ifdef __DLL__
//static HHOOK hhook;
HINSTANCE hinst;
static LPFNNOTIFYCALLBACK lp;
#endif

#ifdef __DLL__
static void LookForOpenFiles( HTASK aTask ) {
   Task_entry* pTE = Task_entry::lookup( aTask ); // nur nachgucken: keine Exception
   if ( pTE == NULL ) return;
   for ( int i = 0; i < File_array_size; i++ ) {
      if ( pTE == File_array[i]._task_entry ) {  // oda die Tasks gleich
        if ( File_array[i]._file != 0 ) {
          File_array[i]._file->close(); // Vorsicht bei blockierenden Calls
          ignore_exception();
          delete File_array[i]._file; File_array[i]._file = 0;
        };
        File_array[i]._task_entry = 0;
      };
   };
   delete pTE; // Die Task iss tot --- es lebe die Task
   pTE = NULL;
};
#endif

// Um mitzukriegen, wann eine Task beendet wird! (WinWord etc.)

#ifdef __DLL__
//extern "C" DWORD SOSDLL
//MsgHookProc( int code, WPARAM wParam, LPARAM lParam )
//{
//  MSG* pMsg = (MSG*) lParam;
//  if ( pMsg->message == WM_DESTROY &&      // Fenster soll zerstoert werden
//       GetParent( pMsg->hwnd ) == NULL ) { // == AppWindow
//      if ( htask != NULL &&
//           htask == GetWindowTask( pMsg->hwnd ) ) {
//        LookForOpenFiles( GetWindowTask( pMsg->hwnd ) );
//        //OutputDebugString( "Applikation htask verabschiedet sich" );
//      };
//  } else {
//
//  };
//  return CallNextHookEx( hhook, code, wParam, lParam );
//};

extern "C" BOOL FAR PASCAL _export
TaskNotifyProc( WORD wID, DWORD dwData ) {
   switch (wID) {
     case NFY_EXITTASK:  LookForOpenFiles( GetCurrentTask() );
                         break;
     //case NFY_STARTTASK: break;
     default: break;
   };
   return 1; // es kann nur einen geben ???
};
#endif

static Dynamic_any_file* lookup_sos_file( Sos_file sos_file )
{
  if ( sos_file > 0 ) {
    if ( sos_file > sizeof File_array_size ) {
      raise( "FILEHDLRANGE", "DLL000" );
    };
#ifdef __DLL__
    if ( GetCurrentTask() != File_array[sos_file-1]._task_entry->Task() ) {
      raise( "WRNGTASK", "DLL000" );
    }
#endif
    return File_array[sos_file-1]._file;
  } else {
    raise( "WRNGFILEHDL", "DLL001" );
  };
exception_handler:
  return 0;
};

static void delete_sos_file( Sos_file sos_file ) {
   Dynamic_any_file* p = File_array[sos_file-1]._file;
   delete p; p = 0;
   File_array[sos_file-1]._file = 0;
#ifdef __DLL__
   File_array[sos_file-1]._task_entry = 0;
#endif
};

void close_all_files() {
static int _all_closed = false;

if (_all_closed) return;
#ifdef __DLL__
//  if ( hhook != NULL ) UnhookWindowsHookEx( hhook ); // Hook wieder loeschen
  if ( lp ) {
    NotifyUnRegister(NULL); // welche Task? egal wg. DLL?
    FreeProcInstance( (FARPROC) lp ); // Instance ooch!
  };
#endif
  for ( int i = 0; i < File_array_size; i ++ ) {
     if ( File_array[i]._file != 0 ) {
       File_array[i]._file->close();
       if ( _XC.raised() ) discard_exception(); // Keine Exceptions !!!
       delete File_array[i]._file;
       File_array[i]._file = 0;
     };
  };
#ifdef __DLL__
  //Task_entry::delete_all();
#endif
  _all_closed = true;
};

//#if !defined(__DLL__)
// #pragma exit close_all_files
//#endif

static Sos_file new_sos_file() {
    static int _init = false;
    for ( int i = 0; i < File_array_size; i++ ) {
       if ( File_array[i]._file == 0 ) break;
    };
    if ( i == File_array_size ) {
       raise( "NOMOREHDL", "DLL002" );
    } else {
      File_array[i]._file = new Dynamic_any_file;
        if ( !File_array[i]._file ) raise( "NOMEMORY", "R101" );
#ifdef __DLL__
      File_array[i]._task_entry = Task_entry::lookup( GetCurrentTask(), TRUE ); xc;
#endif;
      if ( !_init ) {
        // Falls noch nicht initialisiert ...
        init_sos_parameters(); xc; // OHNE StarView!

#ifdef __DLL__
//         hhook = SetWindowsHookEx( WH_GETMESSAGE,
//                                   GetProcAddress( hinst, "MsgHookProc" ),
//                                   hinst,
//                                   NULL ); // erssma systemweit

//         OutputDebugString( "\nMsgHookProc installiert\n" );
        // Windows-Notify Prozedur einrichten
        lp = (LPFNNOTIFYCALLBACK) MakeProcInstance( (FARPROC) TaskNotifyProc, hinst );
        NotifyRegister( NULL, lp, NF_NORMAL );
#else
        atexit( close_all_files ); // in der DLL ber WEP bzw. _exit_dll aufgerufen
#endif
        _init = true;
      };
      return i+1;
    };
  return 0;
exception_handler:
  return 0;
};

extern ostream* log_ptr;

//--------------------------------------------------------------------------
// Ab hier folgt (endlich) die eigentliche Schnittstelle!
//--------------------------------------------------------------------------

// Neuer Exception_Handler (aber nur fuer die DLL)
#if defined(__DLL__)
# define sos_exceptions \
   exception_handler: \
     if (__te) __te->store_exception();
#else
# define sos_exceptions \
   exception_handler: discard_exception();
#endif

#if defined(__DLL__)
# define SOS_BEGIN Task_entry* __te=Task_entry::lookup(GetCurrentTask(), TRUE)

# define sos_return(e) \
   if (__te) __te->store_errno(e); \
   return(e);
#else
 static int _sos_errno;
# define SOS_BEGIN
# define sos_return(e) \
   _sos_errno = e; \
   return(e);
#endif

extern "C" Sos_file SOSDLL
sos_open( const char* file_name,
          Sos_open_mode open_mode ) {
SOS_BEGIN;
#if !defined(__DLL__)
  static int _fs_file_init = false; // schon ein Fs_file erfolgreich offen?
#endif
  Dynamic_any_file* p;
  Sos_file sos_file = new_sos_file(); xc;
  p = File_array[sos_file-1]._file; // statt Lookup!
  // Hack, wg. Ebcdic-Konvertierung
  p->open( file_name, (File_base::Open_mode) open_mode ); xc;
  if ( strlen( file_name ) > 2 &&
       ( !strncmpi( file_name, "nuc", 3 ) ||
         !strncmpi( file_name, "fs", 2 ) ) ) {
    File_array[sos_file-1]._is_nuc = 1;
#if !defined(__DLL__)
    if (!_fs_file_init) {
      atexit(close_all_files); // Damit unbedingt vor Fs_file::disable
      _fs_file_init = true;
    };
#endif
  } else {
    File_array[sos_file-1]._is_nuc = 0;
  };
  if ( log_ptr ) {
    //*log_ptr << "sos_open: erfolgreich" << endl;
  };
  sos_return( sos_file );
sos_exceptions
  delete_sos_file( sos_file );
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_close( Sos_file sos_file ) {
SOS_BEGIN;
  Dynamic_any_file* p = lookup_sos_file( sos_file ); xc;
  p->close(); xc;
  delete_sos_file( sos_file );
  if ( log_ptr ) {
    //*log_ptr << "sos_close: erfolgreich" << endl;
  };
  sos_return( 0 );
sos_exceptions
  delete_sos_file( sos_file );
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_get( Sos_file sos_file, void* buffer, int buffer_size ) {
SOS_BEGIN;
  Area area( buffer, buffer_size );
  int i; unsigned char c;
  char* ptr = (char*) buffer;

  Dynamic_any_file* p = lookup_sos_file( sos_file ); xc;
  p->get( area );

  if ( File_array[sos_file-1]._is_nuc ) {
    xlat( ptr, ptr, area.length(), EBCDIC_TO_ISO );
  };
  xc;

  sos_return( area.length() );
sos_exceptions
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_get_key( Sos_file sos_file, void* buffer, int buffer_size,
             const void* key, const char* key_name ) {
SOS_BEGIN;
  Area area( buffer, buffer_size );
  char key_buf[file_max_key_length];
  int i; unsigned char c;
  int key_length;
  char* ptr = (char*) buffer;

  Dynamic_any_file* p = lookup_sos_file( sos_file ); xc;

  // Key_length bestimmen und Key nach Ebcdic konvertieren
  key_length = p->key_length(); xc;
  if ( File_array[sos_file-1]._is_nuc ) {
    xlat( key_buf, key, key_length, ISO_TO_EBCDIC );
  } else {
    memcpy( key_buf, key, key_length );
  };

  p->get( area, Key( key_buf ) );

  if ( File_array[sos_file-1]._is_nuc ) {
    xlat( ptr, ptr, area.length(), EBCDIC_TO_ISO );
  };
  xc;
  sos_return( area.length() );
sos_exceptions
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_put( Sos_file sos_file, const void* record, int record_length ) {
SOS_BEGIN;
  char* buffer;
  Dynamic_any_file* p;
  int i; char c;

  p = lookup_sos_file( sos_file ); xc;
  buffer = new char[record_length];
    if ( !buffer ) raise( "NOMEMORY", "R101" );
  if ( File_array[sos_file-1]._is_nuc ) {
    xlat( buffer, record, record_length, ISO_TO_EBCDIC );
  } else {
    memcpy( buffer, record, record_length );
  };

  p->put( Const_area( buffer, record_length ) ); xc;

  delete [] buffer; buffer = 0;
  sos_return( 0 );
sos_exceptions
  delete [] buffer; buffer = 0;
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_insert( Sos_file sos_file, const void* record, int record_length ) {
SOS_BEGIN;
  char* buffer;
  Dynamic_any_file* p;
  int i; char c;

  p = lookup_sos_file( sos_file ); xc;
  buffer = new char[record_length];
    if ( !buffer ) raise( "NOMEMORY", "R101" );
  if ( File_array[sos_file-1]._is_nuc ) {
    xlat( buffer, record, record_length, ISO_TO_EBCDIC );
  } else {
    memcpy( buffer, record, record_length );
  };

  p->insert( Const_area( buffer, record_length ) ); xc;

  delete [] buffer; buffer = 0;
  sos_return( 0 );
sos_exceptions
  delete [] buffer; buffer = 0;
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_update( Sos_file sos_file, const void* record, int record_length ) {
SOS_BEGIN;
  char* buffer;
  Dynamic_any_file* p;
  int i; char c;

  p = lookup_sos_file( sos_file ); xc;
  buffer = new char[record_length];
    if ( !buffer ) raise( "NOMEMORY", "R101" );
  if ( File_array[sos_file-1]._is_nuc ) {
    xlat( buffer, record, record_length, ISO_TO_EBCDIC );
  } else {
    memcpy( buffer, record, record_length );
  };

  p->update( Const_area( buffer, record_length ) ); xc;

  delete [] buffer; buffer = 0;
  sos_return( 0 );
sos_exceptions
  delete [] buffer; buffer = 0;
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_store( Sos_file sos_file, const void* record, int record_length ) {
SOS_BEGIN;
  char* buffer;
  Dynamic_any_file* p;
  int i; char c;

  p = lookup_sos_file( sos_file ); xc;
  buffer = new char[record_length];
    if ( !buffer ) raise( "NOMEMORY", "R101" );
  if ( File_array[sos_file-1]._is_nuc ) {
    xlat( buffer, record, record_length, ISO_TO_EBCDIC );
  } else {
    memcpy( buffer, record, record_length );
  };

  p->store( Const_area( buffer, record_length ) ); xc;

  delete [] buffer; buffer = 0;
  sos_return( 0 );
sos_exceptions
  delete [] buffer; buffer = 0;
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_delete_key( Sos_file sos_file, const void* key, const char* key_name ) {
SOS_BEGIN;
  char key_buf[file_max_key_length];
  int i; unsigned char c;
  int key_length;
  char* ptr = (char*) key;

  Dynamic_any_file* p = lookup_sos_file( sos_file ); xc;

  // Key_length bestimmen und Key nach Ebcdic konvertieren
  key_length = p->key_length(); xc;
  if ( File_array[sos_file-1]._is_nuc ) {
    xlat( key_buf, key, key_length, ISO_TO_EBCDIC );
  } else {
    memcpy( key_buf, key, key_length );
  };


  p->del( Key( key_buf ) ); xc;
  sos_return( 0 );
sos_exceptions
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_set_key( Sos_file sos_file, const void* key, const char* key_name ) {
SOS_BEGIN;
  char key_buf[file_max_key_length];
  int i; unsigned char c;
  int key_length;
  char* ptr = (char*) key;

  Dynamic_any_file* p = lookup_sos_file( sos_file ); xc;

  // Key_length bestimmen und Key nach Ebcdic konvertieren
  key_length = p->key_length(); xc;
  if ( File_array[sos_file-1]._is_nuc ) {
    xlat( key_buf, key, key_length, ISO_TO_EBCDIC );
  } else {
    memcpy( key_buf, key, key_length );
  };

  p->set( Key( key_buf ) ); xc;
  sos_return( 0 );
sos_exceptions
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_rename_file( const char* old_name, const char* new_name
                 /*, Sos_file* library */ ) {
SOS_BEGIN;
  Any_base::rename( old_name, new_name ); xc;
  sos_return( 0 );
sos_exceptions
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_delete_file( const char* file_name ) {
SOS_BEGIN;
  Any_base::erase( file_name ); xc;
  sos_return( 0 );
sos_exceptions
  sos_return( -1 );
};

extern "C" int SOSDLL
sos_errno() {
SOS_BEGIN;
#if defined(__DLL__)
  if ( __te ) {
     return __te->errno();
  } else {
     return -2; // interner Fehler!!! abort? Meldung?
  };
#else
  return _sos_errno;
#endif
};

// NEW STYLE
// extern "C" int SOSDLL 
// sos_error_code( char* error_code ) {
// #ifdef __DLL__
//   Task_entry* pTE = Task_entry::lookup( GetCurrentTask(), TRUE );
//   if ( pTE ) {
//     strcpy( error_code, pTE->error_code() );
//   } else {
//     strcpy( error_code, "" );
//   };
// #else
//   strcpy( error_code, _XC.error_code() );
// #endif
//   return 0; 
// };
// 
// extern "C" int SOSDLL 
// sos_exception_name( char* name ) {
// #ifdef __DLL__
//   Task_entry* pTE = Task_entry::lookup( GetCurrentTask(), TRUE );
//   if ( pTE ) {
//     strcpy( name, pTE->exception_name() );
//   } else {
//     strcpy( name, "" );
//   };
// #else
//   strcpy( name, _XC.name() );
// #endif
//   return 0; 
// };

// OLD STYLE
extern "C" const char* SOSDLL 
sos_error_code() {
SOS_BEGIN;
#ifdef __DLL__
  if ( __te ) {
    return __te->error_code();
  } else {
    return "";
  };
#else
  return _XC.error_code();
#endif
};

extern "C" const char* SOSDLL 
sos_exception_name() {
SOS_BEGIN;
#ifdef __DLL__
  if ( __te ) {
    return __te->exception_name();
  } else {
    return "";
  };
#else
  return _XC.name();
#endif
};


#endif
