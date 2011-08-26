// $Id: perl_com.cxx 13638 2008-08-15 13:17:30Z jz $     Joacim Zschimmer

/*
    Dokumenation der Perl-Schnittstelle:

    man perlguts - Introduction to the Perl API
    man perlapi
*/

#include "zschimmer.h"
#include "z_com.h"
#include "log.h"
#include "scripting_engine.h"
#include "perl.h"
#include "perl_com.h"

using namespace std;
using namespace zschimmer::com;


namespace zschimmer {

//--------------------------------------------------------------------------------------perl_com_pm

extern const char perl_com_pm[] =
    "{ "
        "package " Z_PERL_IDISPATCH_PACKAGE_NAME "; "
      //"require 5.005_62; "
        "our $site;"
        "bootstrap " Z_PERL_IDISPATCH_PACKAGE_NAME ";"

        "sub new { shift; my $self = bless {}; $self->{__idispatch} = shift; return $self; } "

        "sub DESTROY "
        "{"
            "my $self = shift; "
            "if( defined $self->{__idispatch} ) { __destroy( $self->{__idispatch} ); undef $self->{__idispatch}; }"
        "}"

        "sub AUTOLOAD "
        "{"
            "my $self = shift; "
            "my $method_name = $AUTOLOAD; "
            "$method_name =~ s/^.*:://; "
            "if( substr($method_name,0,2) eq '__' ) {die;}"
            "$self->{__idispatch} = __resolve_name( $site, $self->{__name} )  unless defined $self->{__idispatch}; " 
            "__call( $self->{__idispatch}, 3, $method_name, @_ ); "
        "}"

        "sub LetProperty "            // Speicherleck: 80 Bytes laufen aus, wenn Perl beendet wird (use_engine="job").
        "{"
            "my $self = shift; "
            "my $property_name = shift; "
            "$self->{__idispatch} = __resolve_name( $site, $self->{__name} )  unless defined $self->{__idispatch}; " 
            "__call( $self->{__idispatch}, 4, $property_name, @_ ); "
        "}"

        "1;"
    "}";

static const string idispatch_member_name = "__idispatch";

template<class T>
inline void unused( const T& ) {}

//-------------------------------------------------------------------------------------bstr_from_pv

static BSTR bstr_from_pv( SV* sv )
{
    STRLEN len = 0;
    const char* s = SvPV( sv, len );
    return bstr_from_string( s, len );
}

//-----------------------------------------------------------------------------XS_Zschimmer_destroy

XS( XS_Zschimmer_destroy )
{
    dXSARGS;

    bool    error = false;
    string  error_text;         // croak() verhindert den Aufruf des Destruktors.

    try
    {
        if( items != 1 )  throw_xc( "Z-PERL-100", "Usage: " Z_PERL_IDISPATCH_PACKAGE_NAME "::__destroy(idispatch)" );

        if( !SvIOK(ST(0)) )  throw_xc( "Z-PERL-100", Z_PERL_IDISPATCH_PACKAGE_NAME "::__destroy()  First argument should be an integer" );

        IDispatch* idispatch = (IDispatch*)(int)SvIV(ST(0));

        if( idispatch )  
        {
            //fprintf( stderr, "perl_com.cxx: %x->Release()\n", (void*)idispatch );
            idispatch->Release();
        }
    }
    catch( const exception&  x ) { error = true;  error_text = x.what(); }
    catch( const _com_error& x ) { error = true;  error_text = string_from_bstr( x.Description() ); }

    if( error )  croak( error_text.c_str() );   // ACHTUNG: croak() kehrt nicht zurück!

    XSRETURN(1);
}

//------------------------------------------------------------------------XS_Zschimmer_resolve_name

XS( XS_Zschimmer_resolve_name )
{
    dXSARGS;

    bool    error = false;
    string  error_text;         // croak() verhindert den Aufruf des Destruktors.

    try
    {
        HRESULT    hr;
        IUnknown*  result = NULL;
        ITypeInfo* type_info;


        if( items != 2 )  throw_xc( "Z-PERL-100", "Usage: " Z_PERL_IDISPATCH_PACKAGE_NAME "::__resolve_name( site, name )" );

        if( !SvIOK(ST(0)) )  throw_xc( "Z-PERL-100", Z_PERL_IDISPATCH_PACKAGE_NAME "::__resolve_name()  First argument should be an integer" );
        if( !SvPOK(ST(1)) )  throw_xc( "Z-PERL-100", Z_PERL_IDISPATCH_PACKAGE_NAME "::__resolve_name()  Second argument should be a string" );

        IActiveScriptSite* site = (IActiveScriptSite*)(int)SvIV(ST(0));
        Bstr name_bstr;
        name_bstr.attach( bstr_from_pv( ST(1) ) );

        hr = site->GetItemInfo( name_bstr, SCRIPTINFO_IUNKNOWN, &result, &type_info );
        if( FAILED(hr) )  throw_com( hr, "GetItemInfo", string_from_bstr(name_bstr).c_str() );

        {
            int RETVAL;
            dXSTARG;
            RETVAL = (int)result;
            XSprePUSH; PUSHi((IV)RETVAL);
        }
    }
    catch( const exception&  x ) { error = true;  error_text = x.what(); }
    catch( const _com_error& x ) { error = true;  error_text = string_from_bstr( x.Description() ); }

    if( error )  croak( error_text.c_str() );   // ACHTUNG: croak() kehrt nicht zurück!

    XSRETURN(1);
}

//-------------------------------------------------------------------------------------------------

static bool sv_to_variant( SV* sv, Variant* variant )
{
    if( SvROK(sv) )  sv = SvRV(sv);     // Referenz?

    switch( SvTYPE( sv ) )
    {
        case SVt_IV:    // Integer?
        {
            *variant = static_cast<int64>( SvIV(sv) );
            //variant->vt = VT_I8;
            //V_I8( variant ) = static_cast<int64>( SvIV(sv) );
            break;
        }
    
        case SVt_NV:    // Double?
        {
            *variant = (double)SvNV(sv);
            //variant->vt = VT_R8;
            //V_R8( variant ) = (double)SvNV(sv);
            break;
        }

        case SVt_PVAV:  // Array?
        {
            AV* av = (AV*)sv;
            int n  = av_len( av ) + 1;  // av_len() liefert nicht die Länge, sondern den höchsten Index
            //int              n         = max( 0, length );

            variant->vt = VT_ARRAY|VT_VARIANT;
            V_ARRAY( variant ) = SafeArrayCreateVector( VT_VARIANT, 0, n );
            if( !V_ARRAY( variant ) )  return false;

            Locked_safearray<Variant> safearray ( V_ARRAY( variant ) );
            
            for( int i = 0; i < n; i++ )
            {
                if( SV** entry = av_fetch( av, i, 0 ) )
                {
                    bool ok = sv_to_variant( *entry, &safearray[ i ] );
                    if( !ok )  return false;
                }
            }

            break;
        }

        case SVt_PVHV:  // Hash value?
        {
            HV* hv = (HV*)sv;

            // Es muss ein IDispatch-Objekt (Z_PERL_IDISPATCH_PACKAGE_NAME) sein!

            SV** sv = hv_fetch( hv, idispatch_member_name.c_str(), idispatch_member_name.length(), 0 );
            if( sv == NULL )  return false;     //throw_xc( "Z-PERL-100", S() << ( i - param_base + 1 ) << ". argument is a hash, which is not acceptable" );
            if( !SvIOK( *sv ) )  return false;  //throw_xc( "Z-PERL-100", "Member '" + idispatch_member_name + "' should be an integer" );

            *variant = reinterpret_cast<IDispatch*>( SvIV( *sv ) );
            //variant->vt = VT_DISPATCH;
            //V_DISPATCH( variant ) = reinterpret_cast<IDispatch*>( SvIV( *sv ) );
            break;
        }
        
        case SVt_NULL:  // Undefined?
        {
            break;
        }

        case SVt_PV:    // String?
        default:        // 2008-08-15
        {
            variant->attach_bstr( bstr_from_pv(sv) );
            //variant->vt = VT_BSTR;
            //V_BSTR( variant ) = attach_bstr( bstr_from_pv(sv) );
            break;
        }

        //default:
        //{
        //    return false;
        //}
    }

    //fprintf( stderr, "perl_com: %d. %s\n", i-param_base, string_from_variant(*variant).c_str());
    return true;
}

//--------------------------------------------------------------------------------XS_Zschimmer_call

XS( XS_Zschimmer_call )
{
    dXSARGS;

    enum Result_type { rt_void, rt_int, rt_double, rt_string, rt_sv };
    Result_type result_type   = rt_void;
    
    IV          int_result    = 0;
    NV          double_result = 0;
    string      string_result;
    SV*         sv_result       = NULL;
    string      error_text;
    bool        error           = false;


    // *** Bei einem Fehler wird croak() gerufen. Diese Routine kehrt nicht zurück. Die Variablen string_result und error_text werden dann nicht freigeben. *** //

    try
    {
        HRESULT             hr;
        DISPID              dispid = 0;
        Dispparams          params;
        Variant             result;
        Excepinfo           excepinfo;
        uint                argnr = (uint)-1;
        string              method_name;
        const int           param_base = 3;


        if( items < param_base )  throw_xc( "Z-PERL-100", "Usage: " Z_PERL_IDISPATCH_PACKAGE_NAME "::__call( idispatch, flags, method, parameter, ... )" );

        if( !SvIOK(ST(0)) || !SvIOK(ST(1)) )  throw_xc( "Z-PERL-100", "Usage: " Z_PERL_IDISPATCH_PACKAGE_NAME "::__call( idispatch, flags, method, parameter, ... )" );

        IDispatch* idispatch = (IDispatch*)(int)SvIV(ST(0));

        if( !idispatch )  throw_xc( "Z-PERL-100", "NULL-Pointer" );

        int flags = (int)SvIV(ST(1));   // DISPATCH_METHOD, DISPATCH_PROPERTYPUT etc.

        if( SvIOK(ST(2)) )
        {
            dispid = (int)SvIV(ST(2));
        }
        else
        if( SvPOK(ST(2)) )  
        {
            method_name = string_from_pv( ST(2) );

            Bstr method_name_bstr = method_name;
            hr = idispatch->GetIDsOfNames( IID_NULL, &method_name_bstr._bstr, 1, STANDARD_LCID, &dispid );
            if( FAILED(hr) )  throw_com( hr, "GetIDsOfNames", method_name.c_str() );
        }
        else
            throw_xc( "Z-PERL-100", "Usage: " Z_PERL_IDISPATCH_PACKAGE_NAME "::__call( idispatch, method, parameter, ... )" );


        params.set_arg_count( items - param_base );

        for( int i = param_base; i < items; i++ )
        {
            bool ok = sv_to_variant( ST(i), &params[ i - param_base ] );
            if( !ok )
            {
                S param_text;
                for( int j = param_base; j < items; j++ )
                {
                    if( j > param_base )  param_text << ",";
                    try
                    {
                        Variant v;
                        bool ok = sv_to_variant( ST(j), &v );
                        
                        if( ok )  
                        {
                            param_text << debug_string_from_variant( v );
                        }
                        else
                        {
                            param_text << "**";

                            SV* sv = ST(j);

                            if( SvROK( sv ) )  // Referenz?
                            {
                                sv = SvRV( sv );
                                param_text << "reference to ";
                            }
                            
                            param_text << "UNKNOWN TYPE CODE " << (int)SvTYPE( sv ) << "**";
                        }
                    }
                    catch( exception& x ) { param_text << x.what(); }
                }

                string kind = flags == DISPATCH_METHOD? "method" : 
                              flags == DISPATCH_PROPERTYPUT? "property (put)" :
                              flags == DISPATCH_PROPERTYGET? "property (get)" : "";

                throw_xc( "Z-PERL-100", S() << kind << " " << method_name << " (" << (string)param_text << "): " << ( i - param_base + 1 ) << ". argument is of unknown type " ); //<< printf_string("0x%X",sv->sv_flags) );
                //throw_xc( "Z-PERL-100", as_string(i+1) + ". parameter is of unknown type " + printf_string("0x%X",sv->sv_flags) );
            }
        }

        if( flags == DISPATCH_PROPERTYPUT )    // 2008-08-15
        {
            params.set_named_arg_count( 1 );
            params.set_dispid( 0, DISPID_PROPERTYPUT );
        }


        hr = idispatch->Invoke( dispid, IID_NULL, STANDARD_LCID, flags, &params, &result, &excepinfo, &argnr );
        if( FAILED(hr) ) 
        {
            if( hr == DISP_E_EXCEPTION )  throw_com_excepinfo( hr, &excepinfo, method_name.c_str(), "" );
        
            string a = method_name;
            if( ( hr == DISP_E_TYPEMISMATCH || hr == DISP_E_PARAMNOTFOUND )  &&  (int)argnr >= 0 )  
                a += ", " + as_string( params.cArgs-argnr ) + ". argument";
            throw_com( hr, "IDispatch::Invoke", a.c_str() );
        }


        switch( result.vt )
        {
            case VT_EMPTY:      break;  
            case VT_NULL:       break;  // NULL ist auch undefined?

            case VT_ERROR:      if( result.scode != DISP_E_PARAMNOTFOUND )    // Das liefert PerlScript, wenn Funktion keine Rückgabe hat.
                                    throw_com( result.scode, "IDispatch::Invoke (result)", method_name.c_str() );
                                break;

            case VT_UNKNOWN:    string_result = printf_string( "IUnknown(0x%X)", (long)V_UNKNOWN(&result) );
                                result_type = rt_string; 
                                break;

            case VT_DISPATCH:   
            {
                IDispatch* idispatch = V_DISPATCH(&result);

                if( idispatch )
                {
                    string expression = "new " Z_PERL_IDISPATCH_PACKAGE_NAME "(" + as_string( (int)idispatch ) + ")";
                    sv_result = eval_pv( expression.c_str(), TRUE );
                    result_type = rt_sv;

                    //fprintf( stderr, "perl_com.cxx: %x->AddRef()\n", (void*)idispatch );
                    idispatch->AddRef();  // DESTROY ruft Release()
                }
                else
                {
                    // Perl "undefined"
                }

                break;
            }

            case VT_INT:        int_result = V_INT ( &result ); result_type = rt_int; break;
          //case VT_UINT:       int_result = V_UINT( &result ); result_type = rt_int; break;
            case VT_I1:         int_result = V_I1  ( &result ); result_type = rt_int; break;
            case VT_UI1:        int_result = V_UI1 ( &result ); result_type = rt_int; break;
            case VT_I2:         int_result = V_I2  ( &result ); result_type = rt_int; break;
            case VT_UI2:        int_result = V_UI2 ( &result ); result_type = rt_int; break;
            case VT_I4:         int_result = V_I4  ( &result ); result_type = rt_int; break;
          //case VT_UI4:        int_result = V_UI4 ( &result ); result_type = rt_int; break;

            case VT_R4:         double_result = V_R4( &result );  result_type = rt_double; break;
            case VT_R8:         double_result = V_R8( &result );  result_type = rt_double; break;

            case VT_BSTR:       string_result = string_from_bstr( V_BSTR( &result ) );
                                result_type = rt_string; 
                                break;

            default:            string_result = string_from_variant( result );
                                result_type = rt_string; 
                                break;
        }
    }
    catch( const exception&  x ) { error = true;  error_text = x.what(); }
    catch( const _com_error& x ) { error = true;  error_text = string_from_bstr( x.Description() ); }

    if( error )  croak( error_text.c_str() );   // ACHTUNG: croak() kehrt nicht zurück!
    //if( error )  result_type = rt_string, string_result = ERROR_ID + error_text;

    //dXSTARG;

    switch( result_type )
    {
        case rt_void: 
            XSRETURN_UNDEF;

        case rt_double:
            //XSprePUSH; 
            //PUSHn( double_result );
            //XSRETURN( 1 );
            XSRETURN_NV( double_result );

        case rt_int:
            //XSprePUSH; 
            //PUSHi( int_result );
            //XSRETURN( 1 );
            XSRETURN_IV( int_result );

        case rt_string:
            //XSprePUSH; 
            //PUSHp( string_result.c_str(), string_result.length() );
            //XSRETURN( 1 );
            XSRETURN_PVN( string_result.c_str(), string_result.length() );

        case rt_sv:
            dXSTARG;
            XSprePUSH; 
            PUSHs( sv_result ); 
            XSRETURN( 1 );
    }
}

//------------------------------------------------------------------------------------perl_com_boot

extern "C" 
XS( perl_com_boot )
{
    dXSARGS;
    unused(items);

    XS_VERSION_BOOTCHECK;

    newXS( Z_PERL_IDISPATCH_PACKAGE_NAME "::__destroy"     , XS_Zschimmer_destroy     , __FILE__ );
    newXS( Z_PERL_IDISPATCH_PACKAGE_NAME "::__resolve_name", XS_Zschimmer_resolve_name, __FILE__ );
    newXS( Z_PERL_IDISPATCH_PACKAGE_NAME "::__call"        , XS_Zschimmer_call        , __FILE__ );

    XSRETURN_YES;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

