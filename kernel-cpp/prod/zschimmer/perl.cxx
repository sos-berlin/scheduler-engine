// $Id$     Joacim Zschimmer

#include "zschimmer.h"
#include "z_com.h"
#include "threads.h"
#include "log.h"
#include "perl.h"
#include "perl_com.h"

extern "C"
{
#   include "../LINKS/perl/CORE/EXTERN.h"
#   include "../LINKS/perl/CORE/perl.h"
}

#if !defined MULTIPLICITY || !defined USE_ITHREADS
#   warning Perl ist nicht konfiguriert mit: bash Configure -Dcc=gcc -Dusethreads -Dusemultiplicity
#endif

/*                                      
    Wie muss das Perl gebaut sein?
    cd .../perl-5.8.0
    bash Configure -Dcc=gcc -Dusethreads -Dusemultiplicity 
    
    Weitere Optionen: 
    -Dprefix=/opt/perl5
    -d (lässt Perl sich selbst antworten)
    -Duseshrplib, um libperl.so zu erzeugen


    bash Configure -Dcc=gcc -Dprefix=/opt/perl-5.8.0 -Dusethreads -Dusemultiplicity -Duseshrplib -d

*/

#undef list


#define PERL ( (PerlInterpreter*)_perl )

namespace zschimmer {

using namespace std;
using namespace zschimmer::com;
using std::list;

/*
static Error_code_text error_codes[] =
{
    { "Z-PERL-", "Wert fehlt (leere Zeichenkette)" },
    { NULL }
};

//-----------------------------------------------------------------------------------zschimmer_init

void zschimmer_init()
{
    add_error_code_texts( error_codes );
}

//------------------------------------------------------------------------------zschimmer_terminate

void zschimmer_terminate()
{
    error_code_map.clear();
}
*/

//---------------------------------------------------------------------------------------throw_perl

void throw_perl( int perl_error, const string& ins )
{
    char error_code[20];

    sprintf( error_code, "PERL-%d", perl_error );
    throw_xc( error_code, ins );
}

//-----------------------------------------------------------------------------------string_from_pv

string string_from_pv( SV* sv )
{
    STRLEN len = 0;
    const char* s = SvPV( sv, len );
    return string( s, len );
}

//--------------------------------------------------------------------------------------Perl::~Perl

Perl::~Perl()
{
    close();
}

//---------------------------------------------------------------------------------------Perl::init

void Perl::init()
{
    close();

    //PL_perl_destruct_level = 1;   // Setting PL_perl_destruct_level to 1 makes everything squeaky clean

    _perl = perl_alloc();
    if( !PERL )  throw_xc( "perl_alloc" );

    Z_LOG( "perl_alloc() ==> " << (void*)_perl << '\n' );

    perl_construct( PERL );
}

//--------------------------------------------------------------------------------------Perl::close

void Perl::close()
{
    if( _perl )
    {
        PERL_SET_CONTEXT( PERL );

#       ifdef Z_SOLARIS
            if( !_start )  _script = "$a=1;",  start();        // Damit perl_destruct() nicht abstürtzt
#       endif

        PL_perl_destruct_level = 1;   // Setting PL_perl_destruct_level to 1 makes everything squeaky clean
        Z_LOG( "perl_destruct(" << (void*)_perl << ")\n" );
        perl_destruct( PERL );
        perl_free( PERL );
        _perl = NULL;
    }
}

//-----------------------------------------------------------------------------Perl::com_class_name

string Perl::com_class_name() const
{ 
    return Z_PERL_IDISPATCH_PACKAGE_NAME; 
}

//------------------------------------------------------------------------------------Perl::xs_init

extern "C" void boot_DynaLoader( pTHX_ CV* cv );

void Perl::xs_init( PerlInterpreter* )
{
    dXSUB_SYS;

    newXS( "DynaLoader::boot_DynaLoader", boot_DynaLoader, __FILE__ );
    newXS( Z_PERL_IDISPATCH_PACKAGE_NAME "::bootstrap", perl_com_boot, __FILE__ );
}

//--------------------------------------------------------------------------------------Perl::parse

void Perl::parse( const string& script_text, VARIANT* result )
{
    if( result )  VariantClear( result );

    PERL_SET_CONTEXT( PERL );

    if( _start )
    {
#if 0
        SV* perl_result = eval_pv( script_text.c_str(), FALSE );   
        if(SvTRUE(ERRSV))  throw_xc( "Z-PERL-100", "eval_pv", string_from_pv(ERRSV).c_str() );

        if( perl_result && result )  fprintf( stderr,"perl_result\n" ), result->vt = VT_BSTR,  result->bstrVal = bstr_from_string( string_from_pv( perl_result ) );
#else
        int         err;
        const char* args[] = { "", "-e", "" };
    
        args[2] = script_text.c_str();

        fprintf( stderr, "perl_parse _perl=%X  %s,%s,%s\n", (int)PERL, args[0], args[1], args[2] );
Z_LOG2( "", "perl_parse(): " << args[2] << "\n" );
        err = perl_parse( PERL, xs_init, 3, (char**)args, NULL );    if(err)  throw_perl( err, "perl_parse" );
Z_LOG2( "", "perl_parse() OK\n" );

        fprintf( stderr, "perl_run\n" );
Z_LOG2( "", "perl_run() ...\n" );
        err = perl_run( PERL );    if(err)  throw_perl( err, "perl_run" );
Z_LOG2( "", "perl_run() OK\n" );
#endif
    }
    else
    {
        if( !_script.empty() )  _script.append( "\n" );
        _script.append( script_text );
    }
}

//--------------------------------------------------------------------------------------Perl::parse

void Perl::eval( const string& expression, VARIANT* result )
{
    VariantClear( result );

    PERL_SET_CONTEXT( PERL );

    SV* perl_result = eval_pv( expression.c_str(), FALSE );   
    if( SvTRUE( ERRSV ) )  throw_xc( "PERL", "eval_pv", string_from_pv(ERRSV).c_str() );

    if( perl_result && result )  result->vt = VT_BSTR,  result->bstrVal = bstr_from_string( string_from_pv( perl_result ) );
}

//--------------------------------------------------------------------------------------Perl::start

void Perl::start( bool start_ )
{
    PERL_SET_CONTEXT( PERL );

    if( !_start )  
    {
        int err;

        //fprintf( stderr, "eval_pv %s\n", script_text.c_str() );
        //err = eval_pv( script_text.c_str(), FALSE );      if(err)  throw_perl(err);
        const char* args[] = { "", "-e", "" };
        args[2] = _script.c_str();

      //fprintf( stderr, "perl_parse _perl=%X  %s,%s,%s\n", (int)PERL, args[0], args[1], args[2] );
      //err = perl_parse( PERL, xs_init, 3, (char**)args, NULL );    if(err)  throw_perl(err,"perl_parse"); //string_from_pv(ERRSV).c_str());
Z_LOG2( "", "perl_parse(): " << _script.c_str() << "\n" );
        err = perl_parse( PERL, xs_init, 3, (char**)args, NULL );    
Z_LOG2( "", "perl_parse() OK\n" );
        
        if( err )
        {
            string perl_error = string_from_pv( ERRSV );
            Z_LOG( "perl_parse():  FEHLER " + as_string(err) + ": " + perl_error + "  in\n" + _script + "\n\n" );
            throw_perl( err, "perl_parse()  " + perl_error );
        }

Z_LOG2( "", "perl_run() ...\n" );
        err = perl_run( PERL );
Z_LOG2( "", "perl_run() OK\n" );

        if( SvTRUE( ERRSV ) )           // ERRSV = $@
        {
            string error = string_from_pv( ERRSV );
            if( error == "" )  error = "(Fehler im Perl-Skript, aber @$ ist leer)";
            throw_xc( "PERL", error );
        }

        if(err)  throw_perl( err, "perl_run()  " + string_from_pv(ERRSV) );
    }

    _start = start_;
}

//---------------------------------------------------------------------------------------Perl::call

void Perl::call( const string& function_name, VARIANT* result )
{
    call( function_name, list<Variant>(), result );
}

//---------------------------------------------------------------------------------------Perl::call

void Perl::call( const string& function_name, const list<Variant>& params, VARIANT* result )
{
    PERL_SET_CONTEXT( PERL );

    string       error;
  //Variant      result;
  //list<string> strings;           // Anker für String-Parameter

    dSP;                            // initialize stack pointer      
    ENTER;                          // everything created after here 
    SAVETMPS;                       // ...is a temporary variable.   
    PUSHMARK(SP);                   // remember the stack pointer    

    for( list<Variant>::const_iterator p = params.begin(); p != params.end(); p++ )
    {
        switch( p->vt )
        {
            case VT_I1:
            case VT_I2:
            case VT_I4:
            case VT_INT:
            case VT_UI1:
            case VT_UI2:
            case VT_UI4:
            case VT_UINT:   
            case VT_I8:
            case VT_UI8:
                XPUSHs( sv_2mortal( newSViv( int64_from_variant( *p ) ) ) );  
                break;

            case VT_R4:
            case VT_R8:     
                XPUSHs( sv_2mortal( newSVnv( double_from_variant( *p ) ) ) );  
                break;

            default:            
            {
                string str = string_from_variant( *p );
              //strings.push_back( str );
                XPUSHs( sv_2mortal( newSVpv( str.c_str(), str.length() ) ) );
            }
        }
    }

    PUTBACK;                        // make local stack pointer global 

    int result_count = perl_call_pv( function_name.c_str(), G_EVAL | G_SCALAR );

    SPAGAIN;                        // refresh stack pointer         

    if( SvTRUE( ERRSV ) )           // ERRSV = $@
    {
        error = string_from_pv( ERRSV );
        if( error == "" )  error = "(Fehler im Perl-Skript, aber @$ ist leer)";
    }


                                    // pop the return value from stack 
    if( result_count-- == 1 )  
    {
        SV* sv = POPs;
        if( sv )
        {
            STRLEN len = 0;
            const char* str = SvPV( sv, len );
            result->vt = VT_BSTR;
            result->bstrVal = bstr_from_string( str, len );
            //fprintf( stderr, "Perl::call result=%s\n", string_from_variant(result).c_str() );
        }
    }

    while( result_count-- > 0 )  POPs; 

    PUTBACK;
    FREETMPS;                       // free that return value        
    LEAVE;                          // ...and the XPUSHed "mortal" args.

    //if(err)  throw_perl(err,string_from_pv(ERRSV).c_str());

    if( error != "" )  
    {
#       ifdef Z_HPUX
            fprintf( stderr, "FEHLER: %s\n", error.c_str() );    // Nach throw stürzt das Programm ab. 12.11.03
#       endif

        throw_xc( "PERL", error );
    }

    //return result;
}

//-------------------------------------------------------------------------------Perl::property_get

void Perl::property_get( const string& property_name, VARIANT* result )
{
    PERL_SET_CONTEXT( PERL );

    perl_get_sv( property_name.c_str(), FALSE );
    
    throw_xc( "Perl::property_get" );
}

//-------------------------------------------------------------------------------Perl::property_put

void Perl::property_put( const string& property_name, const Variant& value )
{
    throw_xc( "Perl::property_put" );
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

