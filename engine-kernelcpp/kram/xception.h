// xception.h
// 26.12.91                                             (c) Joacim Zschimmer

#ifndef __XCEPTION_H
#define __XCEPTION_H

namespace sos
{

struct SOS_CLASS Area;
struct SOS_CLASS Sos_object_base;

const int sos_max_error_code_length = 24;     // ohne 0-Byte
const int max_msg_insertions        = 4;      // Anzahl Einfügungen in Xc
const int max_msg_insertion_length  = 1000;   // jz 10.9.97: Erhöht von 100 auf 100, sossv2.cxx show_msg fügt ggfs. Blanks ein.

struct Const_area;

#define THROW_NONE throw()

//-------------------------------------------------------------------------------------Msg_code

struct Msg_code
{
                                Msg_code                ()                          { _error_code[ 0 ] = 0; }
                                Msg_code                ( const char* error_code );
                                Msg_code                ( const char* code_prefix, const char* code_suffix );
                                Msg_code                ( const char* code_prefix, int numeric_suffix, int width = 1 );

                                operator const char*    () const { return _error_code; }
    int                         numeric_suffix          () const;  // 0, wenn kein numerisches Suffix da

  private:
    char                       _error_code [ sos_max_error_code_length + 1 ];
};

//-----------------------------------------------------------------------------------Source_pos

struct Source_pos
{
                                Source_pos              ( const char* f, int l=-1, int c=-1 )  : _line(l),_col(c) { filename(f); }
                                Source_pos              ( int col = -1 )  : _line(-1),_col(col) { _file[0] = '\0'; }

    int                        _line;                   // 0..n-1, -1: Keine Zeilenangabe
    int                        _col;                    // 0..m-1, -1: Keine Spaltenangabe

    void                        filename                ( const char* f )                 { memcpy( _file, f, min( sizeof _file, strlen(f) + 1 ) ); _file[sizeof _file - 1] = '\0'; }
    const char*                 filename                () const                          { return _file; }
    Bool                        empty                   () const                          { return _file[0] == '\0' && _col == -1; }

  private:
    void                       _assign                  ( const Source_pos& );
    char                       _file[ 50+1 ];
};

extern const Source_pos std_source_pos;

//-------------------------------------------------------------------------------Msg_insertions

struct Xc_base;


struct Msg_insertions //: Sos_self_deleting  für bei Solaris C++ 4.0.1 zum Fehler
{
    /* Einfügungen für den Meldungstext.
       Die Einfügungen werden in dynamisch angefordertem Speicher gehalten.
       Wenn kein Speicher verfügbar ist, werden Einfügungen ignoriert.
    */
                                Msg_insertions          () THROW_NONE;
                                Msg_insertions          ( const char* ) THROW_NONE;
                                Msg_insertions          ( const char*,
                                                          const char*,        // Auf 0 darf kein
                                                          const char* = 0 ) THROW_NONE;  // String folgen!
                                Msg_insertions          ( const Sos_object_base*,
                                                          const char* = 0,    // Auf 0 darf kein
                                                          const char* = 0 ) THROW_NONE;  // String folgen!
                                Msg_insertions          ( const Sos_object_base*,
                                                          const Sos_object_base*,
                                                          const char* = 0 ) THROW_NONE;  // String folgen!
                                Msg_insertions          ( const Msg_insertions& ) THROW_NONE;
                               ~Msg_insertions          () THROW_NONE;

    Msg_insertions&             operator =              ( const Msg_insertions& ) THROW_NONE;

    void                        append                  ( const char*           , Xc_base* = NULL ) THROW_NONE;
    void                        append                  ( const char*, int len  , Xc_base* = NULL ) THROW_NONE;
    void                        append                  ( const Const_area&     , Xc_base* = NULL ) THROW_NONE;
    void                        append_hex              ( const void*, int len  , Xc_base* = NULL ) THROW_NONE;
    void                        append                  ( const Sos_string&     , Xc_base* = NULL ) THROW_NONE;
    void                        append                  ( const Sos_object_base*, Xc_base* = NULL ) THROW_NONE;
    void                        append                  ( int4                  , Xc_base* = NULL ) THROW_NONE;
    const char*                 operator []             ( int i ) const     { return _insertion_array[ i ].c_str(); }
    void                       _obj_print               ( ::std::ostream* ) const THROW_NONE;

  private:
    void                        assign                  ( int, const char*              , Xc_base* = NULL ) THROW_NONE;
    void                        assign                  ( int, const Sos_object_base*   , Xc_base* = NULL ) THROW_NONE;
    void                        assign                  ( int, const char*, int len     , Xc_base* = NULL ) THROW_NONE;
    void                        assign_nolog            ( int, const char*, int len     , Xc_base* = NULL ) THROW_NONE;
    void                       _init                    () THROW_NONE;

    string                     _insertion_array [ max_msg_insertions ];

};

inline ::std::ostream& operator<< ( ::std::ostream&, const Msg_insertions& );

//--------------------------------------------------------------------------------------Xc_base

struct Xc_base : exception
{
    enum Dont_log { dont_log };

                                Xc_base                 ( const char* ) THROW_NONE;
                                Xc_base                 (const char*, Dont_log) THROW_NONE;
                                Xc_base                 ( const char*, const Msg_insertions& ) THROW_NONE;
                                Xc_base                 ( const Xc_base& ) THROW_NONE;
#ifdef SYSTEM_GNU
    virtual                    ~Xc_base                 () throw();                                     // Damit es polymorph wird
#endif
    virtual  void               dummy_virtual           ()                              {}  // Damit es polymorph wird

    void                        assign                  ( const Xc_base& x ) throw();
    void                        insert                  ( const Msg_insertions& ins )           { _insertions = ins; }
    Xc_base&                    insert                  ( const char* s ) throw()               { _insertions.append( s ); return *this; }
    Xc_base&                    insert                  ( const char* s, int len ) throw()      { _insertions.append( s, len ); return *this; }
    Xc_base&                    insert                  ( const Const_area& ) THROW_NONE;
    Xc_base&                    insert_hex              ( const void* s, int len )              { _insertions.append_hex( s, len ); return *this; }
    Xc_base&                    insert                  ( const Sos_string& o )                 { _insertions.append( o ); return *this; }
    Xc_base&                    insert                  ( const Sos_object_base* o )            { _insertions.append( o ); return *this; }
    Xc_base&                    insert                  ( int4 o )                              { _insertions.append( o ); return *this; }

    const char*                 code                    () const  { return _error_code; }
    void                        set_code                ( const char* );
    void                        set_what                ( const string& what )                  { _what = what; }
    int                         numeric_suffix          () const  {return _error_code.numeric_suffix();}  // 0, wenn kein numerisches Suffix da
    void                       _obj_print               ( ::std::ostream* ) const;   // Alle Informationen (von throw)
    void                        print_text              ( ::std::ostream& ) const THROW_NONE;   // Nur Fehlertext
    Bool                        get_text                ( ::std::ostream* ) const THROW_NONE;
    Bool                        get_text                ( Area* ) const THROW_NONE;
    virtual const char*         what                    () const throw();

    const char*                 name                    () const     { return _name; }
    void                        set_name                ( const char* );

    const Source_pos&           pos                     () const     { return _pos; }
    void                        pos                     ( const Source_pos& pos )   { _pos = pos; }

    Source_pos                 _pos;

  protected:
                                Xc_base                 ()                  { _name[ 0 ] = '\0'; }


    void                        log_error               ();
    void                        name                    ( const char* ) THROW_NONE;

    char                       _name [ 20 + 1 ];
    Msg_code                   _error_code;
    Msg_insertions             _insertions;
    string                     _what;
};

//-------------------------------------------------------------------------------------------Xc

struct Xc : Xc_base
{
                                Xc                      ( const char* ) THROW_NONE;
                                Xc                      (const char*, Dont_log) THROW_NONE;
                                Xc                      ( const Msg_code& ) THROW_NONE;
                                Xc                      ( const Msg_code&, const Msg_insertions& ) THROW_NONE;
                                Xc                      ( const Msg_code&, const Msg_insertions&, const Source_pos& ) THROW_NONE;
                                Xc                      ( const exception& ) THROW_NONE;
                                Xc                      ( const Xc& ) THROW_NONE;
                               ~Xc                      () THROW_NONE;
    void                        throw_right_typed       () const;
};

//--------------------------------------------------------------------------------------Xc_copy
// Xc_ptr ohne Referenzzählung, also nur ein Xc_ptr für eine Xc.

struct Xc_copy
{
                                Xc_copy                 ( Xc* x = NULL )                    : _xc(NULL) { set(x); }
                                Xc_copy                 ( const Xc& x )                     : _xc(NULL) { set(x); }
                                Xc_copy                 ( const exception* x )              : _xc(NULL) { set(x); }
                                Xc_copy                 ( const exception& x )              : _xc(NULL) { set(x); }
                                Xc_copy                 ( const Xc_copy& x )                : _xc(NULL) { set(x._xc); }
                               ~Xc_copy                 ()                                  { delete _xc; }

    Xc_copy&                    operator =              ( const Xc_copy& x )                { set(x._xc); return *this; }
    Xc_copy&                    operator =              ( const Xc* x )                     { set(x); return *this; }
    Xc_copy&                    operator =              ( const Xc& x )                     { set(x); return *this; }
    Xc_copy&                    operator =              ( const exception& x )              { set(x); return *this; }
    Xc&                         operator *              () const                            { return *_xc; }
    Xc*                         operator ->             () const                            { return _xc; }
                                operator Xc*            () const                            { return _xc; }
    bool                        operator !              () const                            { return _xc == NULL; }
    friend ostream&             operator <<             ( std::ostream&, const Xc_copy& );
    void                        set                     ( const Xc& );
    void                        set                     ( const Xc* );
    void                        set                     ( const exception& );
    void                        set                     ( const exception* );
    void                    set_time                    ( double t )                        { _time = t; }
    double                      time                    () const                            { return _time; }
    string                      code                    () const                            { return _xc? _xc->code() : ""; }
    string                      what                    () const                            { return _xc? _xc->what() : ""; }

  private:
    Xc*                        _xc;
    double                     _time;
};

//-------------------------------------------------------------------------------------Named_xc

struct Named_xc : Xc
{
                                Named_xc                ( const char* n, const Msg_code& code ) : Xc ( code ) { name( n ); }
};

//-------------------------------------------------------------------------------Standardfehler

// NICHT VERGESSEN:
// Folgende Exception auch in Xc_base::throw_right_typed() eintragen:

struct No_memory_error  : Xc { No_memory_error( const char* e = "R101"     ) THROW_NONE; /*DEFINE_OBJ_COPY( No_memory_error )*/ };
struct Eof_error        : Xc { Eof_error      ( const char* e = "D310"     ) THROW_NONE; /*DEFINE_OBJ_COPY( Eof_error )*/};
struct Not_exist_error  : Xc { Not_exist_error( const char* e = "D140"     ) THROW_NONE; /*DEFINE_OBJ_COPY( Not_exist_error )*/};
struct Exist_error      : Xc { Exist_error    ( const char* e = "D151"     ) THROW_NONE; /*DEFINE_OBJ_COPY( Exist_error )*/};
struct Not_found_error  : Xc { Not_found_error( const char* e = "D311"     ) THROW_NONE; /*DEFINE_OBJ_COPY( Not_found_error )*/};
struct Duplicate_error  : Xc { Duplicate_error( const char* e              ) THROW_NONE; /*DEFINE_OBJ_COPY( Duplicate_error )*/};
struct Too_long_error   : Xc { Too_long_error ( const char* e = "SOS-1113" ) THROW_NONE; /*DEFINE_OBJ_COPY( Too_long_error )*/};
struct No_space_error   : Xc { No_space_error ( const char* e = "D180"     ) THROW_NONE; /*DEFINE_OBJ_COPY( No_space_error )*/};
struct Wrong_type_error : Xc { Wrong_type_error(const char* e              ) THROW_NONE; /*DEFINE_OBJ_COPY( Wrong_type_error )*/};
struct Connection_lost_error : Xc { Connection_lost_error( const char* e              ) THROW_NONE; /*DEFINE_OBJ_COPY( Wrong_type_error )*/};
struct Data_error            : Xc { Data_error           ( const char* e              ) THROW_NONE; /*DEFINE_OBJ_COPY( Wrong_type_error )*/};
struct Locked_error          : Xc { Locked_error         ( const Msg_code& e          ) THROW_NONE; /*DEFINE_OBJ_COPY( Wrong_type_error )*/};
struct Null_error            : Xc { Null_error           ( const char* e = "SOS-1195" ) THROW_NONE; /*DEFINE_OBJ_COPY( Wrong_type_error )*/};
struct Conversion_error      : Xc { Conversion_error     ( const char* e              ) THROW_NONE; /*DEFINE_OBJ_COPY( Wrong_type_error )*/};
struct Overflow_error        : Xc { Overflow_error       ( const char* e = "SOS-1107" ) THROW_NONE; /*DEFINE_OBJ_COPY( Wrong_type_error )*/};

struct Usage_error : Xc
{
    Usage_error( const char* usage, const char* e = "SOS-1225" ) THROW_NONE;
    const char* usage() { return &_usage[0]; }
private:
    char    _usage[256+1];
};

struct Abort_error           : Xc_base { Abort_error() THROW_NONE; };

//---------------------------------------------------------------------------------Syntax_error

struct Syntax_error : Xc
{
                                Syntax_error            ( const char* code, const Msg_insertions& = Msg_insertions(),
                                                          const Source_pos& = Source_pos() ) THROW_NONE;
};

//------------------------------------------------------------------------------------throw_xxx

Z_NORETURN void throw_abort_error           ();
Z_NORETURN void throw_connection_lost_error ( const char* error_code );
Z_NORETURN void throw_data_error            ( const char* error_code );
Z_NORETURN void throw_duplicate_error       ( const char* error_code );
Z_NORETURN void throw_eof_error             ( const char* error_code = "D310" );
Z_NORETURN void throw_eof_error             ( const char* error_code, const Sos_object_base* );
Z_NORETURN void throw_eof_error             ( const char* error_code, int );
Z_NORETURN void throw_exist_error           ( const char* error_code = "D151" );
Z_NORETURN void throw_no_memory_error       ( const char* error_code = "R101" );
Z_NORETURN void throw_no_memory_error       ( unsigned long size );
Z_NORETURN void throw_no_space_error        ( const char* error_code = "D180" );
Z_NORETURN void throw_not_exist_error       ( const char* error_code );
Z_NORETURN void throw_not_exist_error       ( const char* error_code, const Sos_object_base* );
Z_NORETURN void throw_not_exist_error       ( const char* error_code, const char* );
Z_NORETURN void throw_not_found_error       ( const char* error_code = "D311", const Sos_object_base* = 0 );
Z_NORETURN void throw_overflow_error        ( const char* error_code = "SOS-1107" );
Z_NORETURN void throw_overflow_error        ( const char* error_code, const char*, const char* = 0 );
Z_NORETURN void throw_syntax_error          ( const char* e, const Source_pos& pos );
Z_NORETURN void throw_syntax_error          ( const char* error_code, int column = -1 );
Z_NORETURN void throw_syntax_error          ( const char* error_code, const char* insertion, const Source_pos& );
Z_NORETURN void throw_syntax_error          ( const char* error_code, const char* insertion, int column = -1 );
Z_NORETURN void throw_too_long_error        ( const char* error_code = "SOS-1113" );
Z_NORETURN void throw_too_long_error        ( const char* error_code, long, long );
Z_NORETURN void throw_too_long_error        ( const char* error_code, const Sos_object_base* );
Z_NORETURN void throw_wrong_type_error      ( const char* error_code );
Z_NORETURN void throw_null_error            ( const char* error_code, const Sos_object_base* = 0 );
Z_NORETURN void throw_null_error            ( const char* error_code, const char* );
Z_NORETURN void throw_conversion_error      ( const char* error_code, const char* = NULL );

Z_NORETURN void throw_xc( const char* error_code );
Z_NORETURN void throw_xc( const char* error_code, const char* insertion );
Z_NORETURN void throw_xc( const char* error_code, const char* a, const char* b );
Z_NORETURN void throw_xc( const char* error_code, const char* a, int b );
Z_NORETURN void throw_xc( const char* error_code, int, int, int );
Z_NORETURN void throw_xc( const char* error_code, int, int );
Z_NORETURN void throw_xc( const char* error_code, int );
Z_NORETURN void throw_xc( const char* error_code, const Const_area& insertion );
Z_NORETURN void throw_xc( const char* error_code, const Sos_object_base* );
Z_NORETURN void throw_xc( const char* error_code, const Sos_object_base*, int );
Z_NORETURN void throw_xc( const char* error_code, const Sos_object_base*, const char* );
Z_NORETURN void throw_xc( const char* error_code, const Sos_object_base*, const Sos_object_base* );
Z_NORETURN void throw_xc( const char* error_code, const Msg_insertions& a );
Z_NORETURN void throw_xc( const char* error_code, const Msg_insertions& a, const Source_pos& b );
Z_NORETURN void throw_xc( const char* error_code, const Source_pos&, const char*, int, int );
Z_NORETURN void throw_xc( const Xc& );
Z_NORETURN void throw_xc( const exception& );

Z_NORETURN inline void throw_xc( const char* error_code, const string& a )                      { throw_xc( error_code, a.c_str() ); }
Z_NORETURN inline void throw_xc( const char* error_code, const string& a, const string& b )     { throw_xc( error_code, a.c_str(), b.c_str() ); }
Z_NORETURN inline void throw_xc( const char* error_code, const string& a, int b )               { throw_xc( error_code, a.c_str(), b ); }

Z_NORETURN void throw_xc_hex                ( const char* e, const void* p, uint len );

Z_NORETURN void throw_errno                 ( int, const char* function );        // Löst die passende Exception aus
Z_NORETURN void throw_errno                 ( int e, const Msg_insertions& );
Z_NORETURN void throw_errno                 ( int, const char* function, const Sos_object_base* );
Z_NORETURN void throw_errno                 ( int, const char* function, const char* );

Z_NORETURN        void throw_mswin_error    ( const char* function, const char* ins = NULL );
Z_NORETURN inline void throw_mswin_error    ( const char* function, const string& ins )         { throw_mswin_error( function, ins.c_str() ); }
Z_NORETURN        void throw_mswin_error    ( int4 win_error, const char* function, const char* ins = NULL );
Z_NORETURN inline void throw_mswin_error    ( int4 win_error, const char* function, const string& ins )  { throw_mswin_error( win_error, function, ins.c_str() ); }

//---------------------------------------------------------------------------CATCH_AND_THROW_XC

#define CATCH_AND_THROW_XC                                                                  \
    catch( const Xc_base& )     { throw; }                                                  \
    catch( const exception& x ) { throw Xc( x ); }                                          \
    catch( ... )                { throw Xc( "UNKNOWN" ); }


extern void check_new( void* pointer, const char* source = SOURCE );

void                            get_mswin_msg_text      ( Area* buffer, long msg_code );
string                          get_mswin_msg_text      ( long msg_code );

//------------------------------------------------------------------------------------------inlines

inline ::std::ostream& operator<< ( ::std::ostream& s, const Msg_insertions& m )
{
    m._obj_print( &s );
    return s;
}

inline ::std::ostream& operator<< ( ::std::ostream& s, const Xc& x )
{
    x._obj_print( &s );
    return s;
}


} //namespace sos

#endif
