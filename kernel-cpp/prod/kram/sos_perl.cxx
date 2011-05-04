// $Id$     Joacim Zschimmer


#include "precomp.h"
#include "sos.h"
#include "sos_perl.h"


/*
    Für mehrere Instanzen muss Perl mit -DMULTIPLICITY kompiliert sein.
*/


#if defined SYSTEM_UNIX

namespace sos {






//--------------------------------------------------------------------------------------Perl::~Perl

void Perl::~Perl()
{
    perl_destruct( _perl );
    perl_free( _perl );
}

//---------------------------------------------------------------------------------------Perl::init

void Perl::init()
{
}

//---------------------------------------------------------------------------------------Perl::load

void Perl::load( const string& script_text )
{
    char* args[] = { "", "-e", "(source)" };
    args[2] = script_text.c_str();

    perl_parse( _perl, NULL, sizeof args, args, (char**)NULL);
    perl_run( _perl );
}

//---------------------------------------------------------------------------------------Perl::call

Variant Perl::call( const string& name )
{
    char *args[] = { NULL };

    perl_call_argv( name.c_str(), G_DISCARD | G_NOARGS, args );


              perl_eval_pv("$a = 3; $a **= 2", TRUE);
              printf("a = %d\n", SvIV(perl_get_sv("a", FALSE)));

}

//---------------------------------------------------------------------------------------Perl::call

int Perl::call( const string& name, const vector<int> params )
{
    dSP;                            // initialize stack pointer      
    ENTER;                          // everything created after here 
    SAVETMPS;                       // ...is a temporary variable.   
    PUSHMARK(SP);                   // remember the stack pointer    

    for( int i = 0; i < params.size(); i++ )
    {
        XPUSHs( sv_2mortal( newSViv( params[i] ) ) );
    }

    PUTBACK;                        // make local stack pointer global 

    perl_call_pv( name.c_str(), G_EVAL | G_KEEPERR | G_SCALAR )   // call the function             
    // $@ prüfen!

    SPAGAIN;                        // refresh stack pointer         
                                    // pop the return value from stack 
    int result = POPi;

    PUTBACK;
    FREETMPS;                       // free that return value        
    LEAVE;                          // ...and the XPUSHed "mortal" args.

    return result;
}


} //namespace sos

#endif





