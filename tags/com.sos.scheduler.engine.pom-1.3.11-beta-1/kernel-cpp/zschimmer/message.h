// zschimmer.h                                      ©2000 Joacim Zschimmer
// $Id$

#ifndef __ZSCHIMMER_MESSAGE_H
#define __ZSCHIMMER_MESSAGE_H

#include "log.h"


namespace zschimmer {

//------------------------------------------------------------------------------insert_into_message

struct Message_string;
struct Object;

void        insert_into_message( Message_string* m, int index, int64    ) throw();
void        insert_into_message( Message_string* m, int index, const char* ) throw();               // Für gcc
inline void insert_into_message( Message_string* m, int index, long   v ) throw()                   { return insert_into_message( m, index, (int64)v ); }
inline void insert_into_message( Message_string* m, int index, ulong  v ) throw()                   { return insert_into_message( m, index, (int64)v ); }
inline void insert_into_message( Message_string* m, int index, int    v ) throw()                   { return insert_into_message( m, index, (int64)v ); }
inline void insert_into_message( Message_string* m, int index, uint   v ) throw()                   { return insert_into_message( m, index, (int64)v ); }
inline void insert_into_message( Message_string* m, int index, short  v ) throw()                   { return insert_into_message( m, index, (int64)v ); }
inline void insert_into_message( Message_string* m, int index, unsigned short v ) throw()           { return insert_into_message( m, index, (int64)v ); }
//inline void insert_into_message( Message_string* m, int index, size_t v ) throw()                   { return insert_into_message( m, index, (int64)v ); }

void        insert_into_message( Message_string* m, int index, double               ) throw();
void        insert_into_message( Message_string* m, int index, const exception&     ) throw();
void        insert_into_message( Message_string* m, int index, const Object&        ) throw();
void        insert_into_message( Message_string* m, int index, const Object*        ) throw();
void        insert_into_message( Message_string* m, int index, const String_stream& ) throw();

//-----------------------------------------------------------------------------------Message_string

struct Message_string
{
    Message_string()
    :
        _max_insertion_length( (size_t)-1 ),
        _log_level( log_info )
    {
    }

    explicit Message_string( const string& code );

    explicit Message_string( const char* code );

    template< typename T1 >
    Message_string( const string& code, const T1& p1 )
    :
        _max_insertion_length( (size_t)-1 ),
        _log_level( log_info )
    {
        set_code( code );
        insert( 1, p1 );
    }
    
    template< typename T1, typename T2 >
    Message_string( const string& code, const T1& p1, const T2& p2 )
    :
        _max_insertion_length( (size_t)-1 ),
        _log_level( log_info )
    {
        set_code( code );
        insert( 1, p1 );
        insert( 2, p2 );
    }

    template< typename T1, typename T2, typename T3 >
    Message_string( const string& code, const T1& p1, const T2& p2, const T3& p3 )
    :
        _max_insertion_length( (size_t)-1 ),
        _log_level( log_info )
    {
        set_code( code );
        insert( 1, p1 );
        insert( 2, p2 );
        insert( 3, p3 );
    }

    template< typename T1, typename T2, typename T3, typename T4 >
    Message_string( const string& code, const T1& p1, const T2& p2, const T3& p3, const T4& p4 )
    :
        _max_insertion_length( (size_t)-1 ),
        _log_level( log_info )
    {
        set_code( code );
        insert( 1, p1 );
        insert( 2, p2 );
        insert( 3, p3 );
        insert( 4, p4 );
    }

                               ~Message_string              ();


    void                        set_code                    ( const string& );
    void                        set_code                    ( const char* );
    string                      code                        () const                                { return _code; }

    template< typename T >
    void                        insert                      ( int index, const T& value ) throw()   { insert_into_message( this, index, value ); }

    void                        insert                      ( int index, const string& value ) throw()  { insert_string( index, value ); }
    void                        insert                      ( int index, const char*   value ) throw()  { insert_string( index, value ); }

    void                        insert_string               ( int index, const string& ) throw();
    void                        insert_string               ( int index, const char* ) throw();

    void                    set_max_insertion_length        ( size_t length )                       { _max_insertion_length = length; }
    size_t                      max_insertion_length        ();

    void                    set_log_level                   ( Log_level l )                         { _log_level = l; }
    Log_level                   log_level                   () const                                { return _log_level; }

                                operator const string&      () const                                { return _string; }
    const string&               as_string                   () const                                { return _string; }

    Z_NORETURN void             throw_xc                    () const;

  private:
    string                     _code;
    string                     _string;
    size_t                     _max_insertion_length;
    Log_level                  _log_level;
};

//-----------------------------------------------------------------------------------message_string

template< typename CODE_TYPE >
string message_string( const CODE_TYPE& code ) 
{
    Message_string result ( code );
    return result.as_string();
}


template< typename CODE_TYPE, typename T1 >
string message_string( const CODE_TYPE& code, const T1& p1 ) 
{
    Message_string result ( code );

    result.insert( 1, p1 ); 

    return result.as_string();
}


template< typename CODE_TYPE, typename T1, typename T2 >
string message_string( const CODE_TYPE& code, const T1& p1, const T2& p2 ) 
{
    Message_string result ( code );

    result.insert( 1, p1 ); 
    result.insert( 2, p2 ); 

    return result.as_string();
}


template< typename CODE_TYPE, typename T1, typename T2, typename T3 >
string message_string( const CODE_TYPE& code, const T1& p1, const T2& p2, const T3& p3 ) 
{
    Message_string result ( code );

    result.insert( 1, p1 ); 
    result.insert( 2, p2 ); 
    result.insert( 3, p3 ); 

    return result.as_string();
}


template< typename CODE_TYPE, typename T1, typename T2, typename T3, typename T4 >
string message_string( const CODE_TYPE& code, const T1& p1, const T2& p2, const T3& p3, const T4& p4 ) 
{
    Message_string result ( code );

    result.insert( 1, p1 ); 
    result.insert( 2, p2 ); 
    result.insert( 3, p3 ); 
    result.insert( 4, p4 ); 

    return result.as_string();
}

//-----------------------------------------------------------------------------------------throw_xc

Z_NORETURN void throw_xc( const string& code );
Z_NORETURN void throw_xc( const char* code );


template< typename CODE_TYPE, typename T1 >
Z_NORETURN void throw_xc( const CODE_TYPE& code, const T1& p1 ) 
{
    Message_string message ( code );

    message.insert( 1, p1 ); 

    message.throw_xc();
}


template< typename CODE_TYPE, typename T1, typename T2 >
Z_NORETURN void throw_xc( const CODE_TYPE& code, const T1& p1, const T2& p2 ) 
{
    Message_string message ( code );

    message.insert( 1, p1 ); 
    message.insert( 2, p2 ); 

    message.throw_xc();
}


template< typename CODE_TYPE, typename T1, typename T2, typename T3 >
Z_NORETURN void throw_xc( const CODE_TYPE& code, const T1& p1, const T2& p2, const T3& p3 ) 
{
    Message_string message ( code );

    message.insert( 1, p1 ); 
    message.insert( 2, p2 ); 
    message.insert( 3, p3 ); 

    message.throw_xc();
}


template< typename CODE_TYPE, typename T1, typename T2, typename T3, typename T4 >
Z_NORETURN void throw_xc( const CODE_TYPE& code, const T1& p1, const T2& p2, const T3& p3, const T4& p4 ) 
{
    Message_string message ( code );

    message.insert( 1, p1 ); 
    message.insert( 2, p2 ); 
    message.insert( 3, p3 ); 
    message.insert( 4, p4 ); 

    message.throw_xc();
}

//---------------------------------------------------------------------------------------Source_pos

struct Source_pos
{
    explicit                Source_pos                  ( const string& source = "", int line = 0, int col = 0 ) : _source(source), _line(line), _col(col) {}

    string                  to_string                   () const;

    string                 _source;
    int                    _line;
    int                    _col;
};

inline void insert_into_message( Message_string* m, int index, const Source_pos& p ) throw()        { return m->insert_string( index, p.to_string() ); }

//-----------------------------------------------------------------------------------------------Xc

struct Xc : std::exception
{
                            Xc                          ();
    explicit                Xc                          ( const exception& );
  //explicit                Xc                          ( const Xc& x )                             { set_xc( x ); }
    explicit                Xc                          ( const string& code );
    explicit                Xc                          ( const char* code );
    explicit                Xc                          ( const Message_string& );
    explicit                Xc                          ( const _com_error& );                      // Implementiert in z_com.cxx


    template< typename CODE_TYPE, typename T1 >
    Xc( const CODE_TYPE& code, const T1& p1 ) 
    : 
        _code( code ), 
        _return_code(0)
    {
        Message_string message ( code );

        message.insert( 1, p1 ); 

        set_what( message.as_string() );
    }


    template< typename CODE_TYPE, typename T1, typename T2 >
    Xc( const CODE_TYPE& code, const T1& p1, const T2& p2 ) 
    : 
        _code( code ), 
        _return_code(0)
    {
        Message_string message ( code );

        message.insert( 1, p1 ); 
        message.insert( 2, p2 ); 

        set_what( message.as_string() );
    }


    template< typename CODE_TYPE, typename T1, typename T2, typename T3 >
    Xc( const CODE_TYPE& code, const T1& p1, const T2& p2, const T3& p3 ) 
    : 
        _code( code ), 
        _return_code(0)
    {
        Message_string message ( code );

        message.insert( 1, p1 ); 
        message.insert( 2, p2 ); 
        message.insert( 3, p3 ); 

        set_what( message.as_string() );
    }


    template< typename CODE_TYPE, typename T1, typename T2, typename T3, typename T4 >
    Xc( const CODE_TYPE& code, const T1& p1, const T2& p2, const T3& p3, const T4& p4 ) 
    : 
        _code( code ), 
        _return_code(0)
    {
        Message_string message ( code );

        message.insert( 1, p1 ); 
        message.insert( 2, p2 ); 
        message.insert( 3, p3 ); 
        message.insert( 4, p4 ); 

        set_what( message.as_string() );
    }

                            Xc                          ( const char* code, const string& text, const Source_pos& );
                           ~Xc                          () throw();

    Xc&                     operator =                  ( const exception& x )                      { set( x );  return *this; }
    Xc&                     operator =                  ( const Xc& x )                             { set_xc( x );  return *this; }

    void                    set                         ( const exception& );
    void                    set_xc                      ( const Xc& );

    bool                    is_empty                    () const                                    { return _code == ""  &&  _what == ""; }

    const char*             what                        () const throw();
    const string            what2                       () const throw();
    void                set_what                        ( const string& );

    string                  name                        () const                                    { return _name; }
    void                set_name                        ( const string& name )                      { _name = name; }
    
    string                  code                        () const                                    { return _code; }
    void                set_code                        ( const char* );
    void                set_code                        ( const string& code )                      { set_code( code.c_str() ); }
    
    void                    insert                      ( int nr, const char* );
    void                    insert                      ( int nr, const string& s )                 { insert( nr, s.c_str() ); }
    void                    insert                      ( int nr, int );
    void                    append_text                 ( const string& text )                      { append_text( text.c_str() ); }
    void                    append_text                 ( const char* );

    friend ostream&         operator <<                 ( ostream&, const Xc& );


    string                 _name;
    string                 _code;
    string                 _what;
    int                    _return_code;                // Returncode bei Programmende (0: unbestimmt)

  private:
    void                    set_what_by_code            ( const char* code );
};

//----------------------------------------------------------------------------------------Eof_error

//struct Eof_error : Xc
//{
//                            Eof_error                   ( const char* code, const char* text = "" ) : Xc( code, text ) {}
//};

//---------------------------------------------------------------------------Null_pointer_exception

struct Null_pointer_exception : Xc
{
                            Null_pointer_exception      ()                                          : Xc( "NULL-POINTER" ) {}
};

void                        throw_null_pointer_exception();

//-------------------------------------------------------------------------------------------------



/*
Z_NORETURN void             throw_xc                    ( const char* error_code, const char* = NULL, const char* = NULL, const char* = NULL );
Z_NORETURN void             throw_xc                    ( const char* error_code, const string& );
inline Z_NORETURN void      throw_xc                    ( const char* error_code, const string& a, const string& b )    { throw_xc( error_code, a.c_str(), b.c_str() ); }
inline Z_NORETURN void      throw_xc                    ( const char* error_code, const string& a, const string& b, const string& c )    { throw_xc( error_code, a.c_str(), b.c_str(), c.c_str() ); }
Z_NORETURN void             throw_xc                    ( const char* error_code, int64 );
Z_NORETURN void             throw_xc                    ( const char* error_code, int64, int64 );
*/
Z_NORETURN void             throw_xc                    ( const Message_string& );
Z_NORETURN void             throw_xc                    ( const char* error_code, const string&, const Source_pos& );
Z_NORETURN void             throw_xc                    ( const char* error_code, const char*  , const Source_pos& );
Z_NORETURN void             throw_xc                    ( const Xc& );
Z_NORETURN void             throw_eof                   ();
Z_NORETURN void             throw_errno                 ( int errn, const char* = NULL, const char* = NULL, const char* = NULL );
inline Z_NORETURN void      throw_errno                 ( int errn, const string& a )                   { throw_errno( errn, a.c_str(), NULL, NULL ); }
inline Z_NORETURN void      throw_errno                 ( int errn, const string& a, const string& b )  { throw_errno( errn, a.c_str(), b.c_str(), NULL ); }
inline Z_NORETURN void      throw_errno                 ( int errn, const string& a, const string& b, const string& c )  { throw_errno( errn, a.c_str(), b.c_str(), c.c_str() ); }
Z_NORETURN void             throw_socket                ( int errn, const char* = NULL, const char* = NULL, const char* = NULL );
Z_NORETURN void             throw_pattern               ( const char* pattern, int error, const char* function_name, const char* text1 = NULL, const char* text2 = NULL );
Z_NORETURN void             throw_mswin                 ( int mswin_error, const char* function = "", const char* = NULL, const char* = NULL );
inline Z_NORETURN void      throw_mswin                 ( int mswin_error, const char* function, const string& a ) { throw_mswin( mswin_error, function, a.c_str() ); }
inline Z_NORETURN void      throw_mswin                 ( int mswin_error, const char* function, const string& a, const string& b ) { throw_mswin( mswin_error, function, a.c_str(), b.c_str() ); }
Z_NORETURN void             throw_mswin                 ( const char* function = "", const char* ins1 = NULL, const char* ins2 = NULL );
inline Z_NORETURN void      throw_mswin                 ( const char* function, const string& ins1 )                       { throw_mswin( function, ins1.c_str() ); }
inline Z_NORETURN void      throw_mswin                 ( const char* function, const string& a, const string& b )         { throw_mswin( function, a.c_str(), b.c_str() ); }
Z_NORETURN inline void      throw_overflow              ( const char* error_code, const char* a = "", const char* b = NULL )  { throw_xc( error_code, a, b ); }

const char*                 get_error_text              ( const char* code );
inline const char*          get_error_text              ( const string& code )                      { return get_error_text( code.c_str() ); }

string                      get_mswin_msg_text          ( int code );

string                      errno_code                  ( int errn = errno );

//-------------------------------------------------------------------------------------------------

inline void                 Message_string::throw_xc    () const                                    { zschimmer::throw_xc( *this ); }

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
