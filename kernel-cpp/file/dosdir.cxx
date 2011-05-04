//#define MODULE_NAME "dosdir"
// dosdir.cpp

/* Änderungen:
    7. 9.94 jz  Bibliotheksklammern werden ignoriert. Für DIR-Befehl in Fee.
*/

#include "precomp.h"
#include "../kram/sysdep.h"

#if defined __BORLANDC__
#   include <dos.h>             // ????????????
#   include <dir.h>
#   include <mem.h>
#endif

#include <stdlib.h>
#include <string.h>


//#include <headers.h>
#include "../kram/sos.h"
#include "../kram/log.h"

#if defined __BORLANDC__
//#if defined SYSTEM_WIN

// dosdir.h

// Konkrete DosDir-Klasse ...
// Usage: <dirname>[<match>] {<opt>}
// <dirname> :== [<drive>:]<dirname2> | . | ..
// <drive> :== [a-z]
// <dirname2> :== \ | . | .. | <filename>\<dirname2>
// <filename> :== [a-z*?]^8.[a-z*?]^3
// <match> :== <filename>
// <opt> :== -r | -f | -d | -i
// Optionen bedeuten:
// -r: rekursiv durchsuchen und matchen
// -f: Fullname ausgeben
// -d: Directory-Flag. Ohne -r muessen die Dirs matchen, mit -r
//     werden sie auch ohne Matching ausgegeben.
// -r: falls nicht -f und -d sowie -r gesetzt ist, werden die
//    Files & Dirs eingerueckt(mit 2 Blanks pro Ebene).

#include <dospath.h>
#include <dos.h>
#include <absfile.h>


#define DIR_MAX_OPTIONS 20

#define DIR_FULLNAME  1
#define DIR_REKURSIV  2
#define DIR_DIRECTORY 4
#define DIR_ALL       8
#define DIR_INDENT    16

#define is_set(arg) (arg & _flags)

namespace sos {

struct Dosdir : public Abs_file
{
    Dosdir() { _init(); };

                               ~Dosdir                 ();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode  );

    // Dosdir-spezifische Funktionen ...
    int                         eof                     () { return _eof; };
    const char*                 pwd                     () { return _dirname; };
    static Bool                 is_dir                  ( const char* d ) { return check_dir(d); };
    static int                  check_dir               ( const char* dirname );

protected:
    void                        get_record              ( Area &area );

private:
    static int                  is_root( const char* dirname );

   void _init() {
     _eof = _opened = _flags = _dir_searched = 0;
     _dir = 0;
   };
   void open_rec ( const char* dirname, //const char *rel_dirname,
           const char* match, int flags );
   void parse_flags( const char* options );
   int  is_point_dir( const char* fname );
   void open_first();
   void get_next( int first_time = false );
   void make_next( const char* name, int dir = false );
   void get_next_dir( int first_time = false );

   find_t _current;
   Sos_ptr<Dosdir> _dir;
   int _eof;
   int _opened;
   int _flags;
   int _dir_searched;
   char _name[_MAX_PATH];
   char _dirname[_MAX_DIR];
   char _match[_MAX_FNAME+_MAX_EXT];
};

// Implementation ...

#include "dosdir.h"

//--------------------------------------------------------------Dosdir_file_type

struct Dosdir_file_type : Abs_file_type
{
    Dosdir_file_type() : Abs_file_type() {};

    virtual const char* name() const { return "dir"; }
  //virtual const char* alias_name() const { return "dosdir"; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Dosdir> f = SOS_NEW_PTR( Dosdir );
        return +f;
    }
};


const Dosdir_file_type  _dir_file_type;
const Abs_file_type&     dir_file_type = _dir_file_type;



//-----------------------------------------------------------------Dosdir::~Dosdir

Dosdir::~Dosdir()
{
}

//---------------------------------------------------------Dosdir::parse_flags
void Dosdir::parse_flags ( const char* options ) {

   for ( const char *ptr = strchr(options,'-'); ptr; ptr = strchr( ptr+1, '-') ) {
       switch (*(ptr+1)) {
     case 'f': _flags |= DIR_FULLNAME; break;
     case 'd': _flags |= DIR_DIRECTORY; break;
     case 'r': _flags |= DIR_REKURSIV; break;
     //case 'a': _flags |= DIR_ALL; break;
     case 'i': _flags |= DIR_INDENT; break;
     default:  //cerr << "Unknown Dosdir flag (ignored): " << *(ptr+1) << endl;
              break;
       }
   }
   if ( is_set(DIR_INDENT) &&
    ( !is_set(DIR_REKURSIV) || !is_set(DIR_DIRECTORY)
      || is_set(DIR_FULLNAME) )) {
       _flags ^= DIR_INDENT;
       //cerr << "Use Option -i only with -d and -r and NOT -f." << endl;
       throw( "DOSDIR" );
   }
}

//----------------------------------------------------------------Dosdir::open
void Dosdir::open( const char* filename, Open_mode open_mode, const File_spec& )
{
   char token[_MAX_PATH+DIR_MAX_OPTIONS], buf[_MAX_PATH];
   char * ptr;


   if ( open_mode & out )  throw_xc( "D1??" ); //raise("OPENMODE", "D1??");
   if ( strlen( filename ) > _MAX_PATH+DIR_MAX_OPTIONS )  throw_too_long_error();
       //raise("DIR2LONG", "D1??");


   strcpy( token, filename );

   
   if( filename[ 0 ] == '(' ) {
       // Bibliotheksklammern ohne Bibliothek entfernen:
       strcpy( token, filename + 1 );
       for( char* p = token + strlen( token ) - 1; p > token; p-- )  if( *p != ' ' )  break;
       if( *p == ')' )  *p = 0;             
   }
   
   _flags = 0;
   ptr = strtok( token, " " );
   strupr( token );
   if (ptr) {
     parse_flags( strchr( ptr, 0) + 1);
   };

   if (!token)   throw_xc( "DIRNAME" );

   if (!strcmp( token, ".") || !strcmp( token, "..")) {
       strcat( token, "\\*.*" );
   } else if ( '\\' == token[strlen(token) - 1] ) {
       strcat( token, "*.*" );
   }

   if (!_fullpath( buf, token, _MAX_PATH ))   throw_xc( "DIRNAME" );

   { char  drive[_MAX_DRIVE], dir[_MAX_DIR],
       name[_MAX_FNAME], ext[_MAX_EXT];

     fnsplit( buf, drive, dir, name, ext );
     strcpy( _dirname, drive ); strcat( _dirname, dir );
     strcpy( _match, name ); strcat( _match, ext );
   }
   if (!check_dir( _dirname )) {
       //cerr << "Wrong Dirname: " << _dirname << endl;
       throw_not_exist_error( "D140" );  //raise("NOTEXIST", "D140");
   }
   if ( !is_set(DIR_REKURSIV)) _dir_searched = true;
   open_first();
   _opened = true;

exceptions
}

//------------------------------------------------------------Dosdir::open_rec
void Dosdir::open_rec( const char* dirname, //const char *rel_dirname,
            const char* match,
                    int flags )
{

   strcpy( _dirname, dirname );
   //strcpy( _rel_dirname, rel_dirname );
   strcpy( _match, match );
   if ( flags & DIR_FULLNAME ) {
     _flags = flags ^ DIR_FULLNAME;
   } else {
     _flags = flags;
   };
   open_first();
   _opened = true;
}

//----------------------------------------------------------Dosdir::open_first
// Holen des ersten sowie der weiteren Eintr„ge ...
void Dosdir::open_first() {
  get_next(true);
}

//-----------------------------------------------------------Dosdir::make_next
void Dosdir::make_next( const char* name, int dir ) {
   if (_eof) return;

   memset(_name, 0, sizeof _name);
   if is_set(DIR_FULLNAME) {
      strcpy( _name, _dirname );
   };
   strcat( _name, name );

   if ( dir )
      strcat( _name, "\\" );
}

//--------------------------------------------------------Dosdir::get_next_dir
void Dosdir::get_next_dir ( int first_time ) {
  if ( first_time ) {
    char buf[_MAX_PATH];

    strcpy( buf, _dirname ); strcat( buf, "*.*" );
    _eof = _dos_findfirst( buf, FA_DIREC, &_current );
  } else {
    _eof = _dos_findnext( &_current );
  };

  int ready = false;
  while (!ready) {
    if ( _eof )
      ready = true;
    else if (( _current.attrib & FA_DIREC )
         && !is_point_dir( _current.name )) {
      ready = true;
    } else {
      _eof = _dos_findnext( &_current );
    };
  };

  if (_eof) {
    _dir_searched = true;
  } else {
    char buf[_MAX_PATH];

    strcpy( buf, _dirname ); strcat( buf, _current.name );
    strcat( buf, "\\" );
    _dir = SOS_NEW_PTR( Dosdir );
       if (!_dir)   throw_no_memory_error(); //raise ( "NOMEMORY", "R101" );
    _dir->open_rec( buf, _match, _flags ); xc;
  }
exceptions
}

//------------------------------------------------------------Dosdir::get_next
void Dosdir::get_next( int first_time )
{
try {
   // Note: falls DIR_DIRECTORY gesetzt gilt folgendes:
   //       - falls DIR_REKURSIV: wird ausgegeben.
   //       - sonst: muss Match erfllen.

   if ( first_time && is_set(DIR_REKURSIV)
    && !_dir_searched ) {
      // erste Dir rekursiv durchforsten ...
      get_next_dir( first_time ); xc;
      if (_eof) {
    get_next( true ); xc; return;
      };
      if is_set(DIR_DIRECTORY) {
      make_next( _current.name, true ); return;
      }
   };
   if ( _dir && is_set(DIR_REKURSIV) && !_dir_searched ) {
      // weiter mit Dirs ...
      if (!_dir->eof()) {
        Record_length length, slen;
         char buf[_MAX_PATH];

     if ( is_set(DIR_INDENT)) {
        strcpy( buf, "  " );
     } else {
        strcpy( buf, _current.name ); strcat( buf, "\\" );
     };
     slen = strlen( buf );
     //_dir->get( &buf[slen], (sizeof buf) - slen, &length ); xc;
     _dir->get_record( Area( &buf[slen], (sizeof buf) - slen ) ); xc;
     make_next( buf, false );
      } else {
     SOS_DELETE( _dir );
     get_next_dir(); xc;
     if ( is_set(DIR_DIRECTORY)) {
        if (_eof) {
           get_next( true ); xc; return;
            } else {
               make_next( _current.name, true ); return;
        };
         } else {
            while( !_eof && _dir->eof() ) {
          get_next_dir(); xc;
        };
        if (_eof) {  // Keine Dirs mehr ...
          get_next( true ); xc; return;
        } else {    // es gibt eine mit nichtleerem Inhalt ...
          get_next(); xc; return;
        }
     };
      };
      return;
   };
   if ( first_time && _dir_searched ) {
     char buf[_MAX_PATH];
     int  attrib = _A_NORMAL;

     if ( is_set(DIR_DIRECTORY) && !is_set(DIR_REKURSIV))
       attrib |= FA_DIREC;

     strcpy( buf, _dirname ); strcat( buf, _match );
     _eof = _dos_findfirst( buf, attrib, &_current );
   } else {
     _eof = _dos_findnext( &_current );
   }
   if (_eof) return;

   if (!is_set(DIR_REKURSIV) && !_eof
       && !is_set(DIR_ALL)
       && is_point_dir( _current.name )) {
     get_next(); xc; // .-Dateien ignorieren ...
     return;
   }
   make_next( _current.name, _current.attrib & FA_DIREC );
}
catch(...) {
   SOS_DELETE( _dir );
   throw;
}}

//-----------------------------------------------------------------Dosdir::get_record
// Liefern des aktuellen Eintrages ...
void Dosdir::get_record( Area &area)
{
try {
   uint length;

   if (_eof)   throw_eof_error( "D310" );  //raise("EOF", "D310");

   //length = strlen(_name) + 1;
   //memcpy( area.ptr(), _name, min( area.size(), length ) );
   //area.set_length( length );
   area.assign( _name );
   get_next(); xc;

   //if ( length > area.size() )  throw_too_long_error( "D320" );//raise("TRUNCATE", "D320");
}
catch(...) {
   SOS_DELETE( _dir );
   throw;
}}

//---------------------------------------------------------------Dosdir::close
void Dosdir::close ( Close_mode mode )
{
try {
    if (_dir) {
       _dir->close( mode ); xc;
       SOS_DELETE( _dir );
    }
}
catch(...) {
   SOS_DELETE( _dir );
   throw;
}}

//--------------------------------------------------------Dosdir::is_point_dir
int Dosdir::is_point_dir ( const char* fname ) {

   return ( strcmp( fname, "." )  == 0 )
       || ( strcmp( fname, ".." ) == 0 );
}

//-------------------------------------------------------------Dosdir::is_root
int Dosdir::is_root ( const char* dirname ) {
   // hack um \ zu erkennen!!!
   if ( strlen( dirname ) == 3 && dirname[2] == '\\' )
     return true;
   else return false;
}

//-----------------------------------------------------------Dosdir::check_dir
int Dosdir::check_dir ( const char* dirname ) {
   if ( is_root( dirname ) ) return true;

   char buf[_MAX_DIR+1];
   find_t f;

   int len = strlen( dirname );
   memset( buf, 0, _MAX_DIR+1 );
   if ( len > 0 && (dirname[len-1] == '\\') ) {
     strncpy( buf, dirname, len - 1 );
   } else {
     strncpy( buf, dirname, len );
   };
   unsigned int ret =_dos_findfirst( buf, FA_DIREC, &f );
if (log_ptr) *log_ptr << "check_dir: f=" << dirname << ", ret="
                      << ret << ", errno=" << errno << endl;
   return (ret==0?true:false);
}

// ------------------------------------------------------------- dirname_exist

Bool dirname_exist( const char* dirname ) 
{
    return Bool( Dosdir::check_dir( dirname ) );
}


} //namespace sos

#endif
