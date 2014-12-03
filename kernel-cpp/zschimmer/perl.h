// $Id: perl.h 11394 2005-04-03 08:30:29Z jz $    Joacim Zschimmer

#ifndef __Z_PERL_H
#define __Z_PERL_H

#include <list>
#include "z_com.h"
#include "perl_com.h"

namespace zschimmer
{

//void                            throw_perl              ( int perl_error );
string                          string_from_pv          ( SV* );

//---------------------------------------------------------------------------------------------Perl

struct Perl : Object
{
                                Perl                    ()                                          : _zero_(this+1) {}
                               ~Perl                    ();

    void                        init                    ();
    void                        close                   ();
    void                        parse                   ( const string& script_text, VARIANT* result = NULL );
    void                        start                   ( bool start_ = true );

    void                        eval                    ( const string& expression, VARIANT* result );
 //?Variant                     eval                    ( const string& expression )                { Variant result; eval( expression, &result ); return result; }
    

  //com::Variant                call                    ( const string& function_name );
  //com::Variant                call                    ( const string& function_name, const std::list<com::Variant>& par );
  //com::Variant                property_get            ( const string& property_name );
    void                        call                    ( const string& function_name, VARIANT* result );
    void                        call                    ( const string& function_name, const std::list<com::Variant>& par, VARIANT* result );
    void                        property_get            ( const string& property_name, VARIANT* result );
    void                        property_put            ( const string& property_name, const com::Variant& );
    string                      com_class_name          () const;

  private:
    static void                 xs_init                 ( PerlInterpreter* );
  //static void                 xs_init                 ();  // ohne MULTIPLICITY

    Fill_zero                  _zero_;
    void*                      _perl;
    bool                       _start;
    string                     _script;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer


#endif
