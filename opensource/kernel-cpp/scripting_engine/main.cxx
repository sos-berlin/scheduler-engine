// $Id: main.cxx 13498 2008-04-17 08:34:06Z jz $

#include "zschimmer/zschimmer.h"
#include "zschimmer/log.h"
#include "zschimmer/file.h"
#include "zschimmer/file_path.h"
#include "zschimmer/z_com_server.h"
#include "zschimmer/scripting_engine.h"
#include "spidermonkey.h"

#ifdef Z_WINDOWS
#   include <io.h>
#endif

using namespace std;
using namespace zschimmer;
using namespace zschimmer::file;
using namespace zschimmer::com;

//---------------------------------------------------------------------------------Script_exception

struct Script_exception : Xc
{
                                Script_exception        ( const string& message, const Source_pos& pos ) : Xc( "SCRIPT-ERROR", message.c_str(), pos.to_string() ), _message(message) {} 
                               ~Script_exception        ()  throw()                                {}

    string                     _message;
};

//--------------------------------------------------------------------------------------Script_site

struct Script_site : iunknown_implementation< Script_site, IActiveScriptSite >
{                                      
    Fill_zero _zero_;

                                Script_site             ()                                          : _zero_(this+1) {}
                               ~Script_site             ();

    void                        init                    ( const CLSID& );
    void                        close                   ();
    void                        parse                   ( const string& source_text, const string& source_name, int flags = SCRIPTTEXT_ISVISIBLE, VARIANT* result = NULL );



    // IUnknown:
    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    // IActiveScriptSite:
    STDMETHODIMP                GetDocVersionString     ( BSTR *pbstrVersion ) 	                        { *pbstrVersion = SysAllocString( L"" ); return S_OK; }
    STDMETHODIMP                GetItemInfo             ( LPCOLESTR, DWORD, IUnknown**, ITypeInfo** )   { return E_FAIL; }
    STDMETHODIMP                OnScriptError           ( IActiveScriptError* );
    STDMETHODIMP                GetLCID                 ( LCID *plcid )	                                { *plcid = 9; return S_OK; }
    STDMETHODIMP                OnScriptTerminate       ( const VARIANT *, const EXCEPINFO * )          { return S_OK; }
    STDMETHODIMP                OnStateChange           ( SCRIPTSTATE )                                 { return S_OK; }
    STDMETHODIMP                OnEnterScript           ()                                              { return S_OK; }
    STDMETHODIMP                OnLeaveScript           ()                                              { return S_OK; }


    string                     _source_name;
    int                        _first_lineno;           // Wird auf die Zeilennummer der Fehlermeldung der Scripting Engine addiert
    ptr<IActiveScriptParse>    _script_parse;
    ptr<IActiveScript>         _script;
    Xc*                        _script_exception;
};


//-----------------------------------------------------------------------Script_site::OnScriptError

STDMETHODIMP Script_site::OnScriptError( IActiveScriptError* script_error )
{
    DWORD       dwCookie    = 0;
    LONG        column_no   = 0;
    ULONG       line_no     = 0;
    Bstr        source_line;
    string      source;
    Excepinfo   excepinfo; 
    string      description;
    
    memset( &excepinfo, 0, sizeof excepinfo );

    script_error->GetSourcePosition( &dwCookie, &line_no, &column_no );
    script_error->GetSourceLineText( &source_line );
    script_error->GetExceptionInfo( &excepinfo );

    source = string_from_bstr( excepinfo.bstrSource );

    if( source_line )  source = ", scriptline=" + string_from_bstr( source_line );

    delete _script_exception;  _script_exception = NULL;

    string descr = string_from_bstr( excepinfo.bstrDescription );
    
    if( _source_name != "" )  source = _source_name + " " + source;


    Source_pos source_pos ( source, line_no, column_no );

    if( excepinfo.scode )
    {
        char code[20]; 
        z_snprintf( code, sizeof code, "COM-%08X", excepinfo.scode );
        _script_exception = new Xc( code, descr, source_pos );
    }
    else
    {
        _script_exception = new Script_exception( descr, source_pos );
    }

    if( !_script_exception )  throw_xc( "NO_MEMORY" );

    return E_FAIL;
}

//------------------------------------------------------------------------Script_site::~Script_site

Script_site::~Script_site()
{
    delete _script_exception;  _script_exception = NULL;
}

//----------------------------------------------------------------------Script_site::QueryInterface

STDMETHODIMP Script_site::QueryInterface( const IID& iid, void** result )
{ 
    Z_IMPLEMENT_QUERY_INTERFACE2( this, iid, IID_IActiveScriptSite, IActiveScriptSite, result );

    return Iunknown_implementation::QueryInterface( iid, result ); 
}

//---------------------------------------------------------------------------------cript_site::init

void Script_site::init( const CLSID& clsid )
{
    HRESULT hr;

    #ifdef Z_WINDOWS
    if( clsid == CLSID_Spidermonkey )
    {
        _script_parse = static_cast<IActiveScriptParse*>( com_create_instance( CLSID_Spidermonkey, IID_IActiveScriptParse, NULL, "spidermonkey" ) );
    }
    else
    #endif
    {
        _script_parse.create_instance( clsid );
    }

    hr = _script_parse->InitNew();
    if( FAILED(hr) )  throw_com( hr, "IActiveScriptParse::InitNew" );


    hr = _script.Assign_qi( _script_parse, IID_IActiveScript );
    if( FAILED(hr) )  throw_com( hr, "IActiveScriptParse::QueryInterface" );


    hr = _script->SetScriptSite( this );
    if( FAILED(hr) )  throw_com( hr, "IActiveScript::SetScriptSite" );


    hr = _script->SetScriptState( SCRIPTSTATE_STARTED );    
    if( FAILED(hr) )  throw_com( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_STARTED" );
}

//-------------------------------------------------------------------------------Script_site::close

void Script_site::close()
{
    HRESULT hr;


    if( _script ) 
    { 
        hr = _script->Close();
        if( FAILED( hr ) )  Z_LOG( "Script_site::close_engine: IActiveScript::Close versagt\n" );

        _script = NULL; 
    }

    if( _script_parse ) 
    { 
        Z_LOGI( "Script_site::close_engine()  Release script\n" );

        _script_parse = NULL; 
    }
}

//-------------------------------------------------------------------------------Script_site::parse

void Script_site::parse( const string& source_text, const string& source_name, int flags, VARIANT* result )
{
    HRESULT     hr;
    Variant     my_result;
    Bstr        source_text_bstr = source_text;
    Excepinfo   excepinfo;

    delete _script_exception;  _script_exception = NULL;

    _source_name = source_name;

    if( !result )  result = &my_result;

    hr = _script_parse->ParseScriptText( source_text_bstr,
                                         NULL, 
                                         NULL, 
                                         NULL, 
                                         0, 
                                         0,          // line number
                                         flags,      // SCRIPTTEXT_ISVISIBLE, 
                                         result,     // Nur für SCRIPTTEXT_ISEXPRESSION
                                         &excepinfo
                                       );

    if( _script_exception )  
    {
        if( Script_exception* x = dynamic_cast<Script_exception*>( _script_exception ) )  throw *x;
        throw *_script_exception;
    }

    if( FAILED( hr ) ) 
    {
        if( !excepinfo.bstrSource  ||  !excepinfo.bstrSource[0] )  String_to_bstr( _source_name, &excepinfo.bstrSource );
        throw_com_excepinfo( hr, &excepinfo, "IActiveScriptParse::ParseScriptText", _source_name.c_str() ); 
    }
}

//------------------------------------------------------------------------------------------unquote

static string unquote( const string& value )
{
    if( value.length() >= 2  &&  *value.begin() == *value.rbegin() )
    {
        string result;
        result.reserve( value.length() - 2 );

        for( size_t i = 1; i < value.length() - 1; i++ )
        {
            if( value[i] == '\\'  &&  i+1 < value.length()  &&  value[i+1] == value[0] )  continue;     // ?
            result += value[i];
        }

        return result;
    }
    else
        return value;
}

//--------------------------------------------------------------------------------------Script_part

struct Script_part
{
                                Script_part                 ( const File_path& filename )            : _filename(filename), _source_name( filename.path() ) {}
                                Script_part                 ( const string& script_text, const string& source_name ) : _script_text(script_text), _source_name(source_name) {}

    File_path                  _filename;
    string                     _script_text;
    string                     _source_name;
};

//---------------------------------------------------------------------------------------------main

int main( int argc, char** argv )
{
    vector<Script_part> script_parts;
    int                 script_option_counter   = 0;
    bool                has_unnamed_argv        = false;
    File_path           script_filename;
    bool                interactive             = false;
    bool                use_stdin               = false;
    Script_site         script_site;
    HRESULT             hr;


    hr = CoInitialize( NULL );
    if( FAILED( hr ) )  throw_com( hr, "CoInitialize" );

    try
    {
        if( argc == 1 )     // Ganz ohne Parameter?
        {
            if( isatty( 0 ) )  interactive = true;
                         else  use_stdin = true;
        }

        for( int i = 1; i < argc; i++ )
        {
            string arg = argv[i];

            if( string_begins_with( arg, "-" ) )
            {
                size_t e = arg.find( "=" );
                e = e == string::npos? arg.length() : e + 1;
                string option_name  = arg.substr( 0, e );
                string option_value = unquote( arg.substr( e ) );

                if( arg == "-i"  
                 || arg == "-interactive" )
                {
                    interactive = true;
                }
                else
                if( option_name == "-script=" )
                {
                    script_parts.push_back( Script_part( option_value, as_string( ++script_option_counter ) + ". -script=" ) );
                }
                else
                if( option_name == "-include=" )
                {
                    script_parts.push_back( File_path( option_value ) );
                }
                else
                if( option_name == "-sos.ini=" )        // Für new ActiveXObject( "hostware.File" )
                {
                    set_environment_variable( "SOS_INI", option_value );
                }
                else
                if( arg == "-" )
                {
                    if( has_unnamed_argv )  goto USAGE;
                    has_unnamed_argv = true;

                    use_stdin = true;
                }
                else 
                    goto USAGE;
            }
            else
            {
                if( has_unnamed_argv )  goto USAGE;
                has_unnamed_argv = true;

                script_filename = argv[ i ];
                //script_parts.push_back( File_path( argv[i] ) );
            }
        }


        script_site.init( CLSID_Spidermonkey );


        Z_FOR_EACH( vector<Script_part>, script_parts, sp )
        {
            if( sp->_script_text != "" )
            {
                script_site.parse( sp->_script_text, sp->_source_name );
            }
            else
            {
                script_site.parse( string_from_file( sp->_filename ), sp->_source_name );
            }
        }

        if( script_filename != "" )
        {
            string script_text = string_from_file( script_filename );
            
            if( string_begins_with( script_text, "#!" ) )
            {
                size_t n = script_text.find( '\n' );
                if( n == string::npos )  n = script_text.length();
                script_text.erase( 0, n );
            }

            script_site.parse( script_text, script_filename );
        }

        if( use_stdin )
        {
            script_site.parse( string_from_fileno( 0 ), "(stdin)" );
        }


        if( interactive )
        {
            bool write_end_of_line_to_stderr = false;

            while(1)
            {
                fputs( "javascript> ", stderr );
                write_end_of_line_to_stderr = true;

                char  line [ 16*1024 ];
                char* ret = fgets( line, sizeof line, stdin );
                if( !ret )  break;

                write_end_of_line_to_stderr = false;

                try
                {
                    Variant result;
                    script_site.parse( line, "(stdin)", SCRIPTTEXT_ISVISIBLE | SCRIPTTEXT_ISEXPRESSION, &result );
                    cout << result << "\n";
                }
                catch( Script_exception& x )
                {
                    cout << "Fehler  " << x._message << "\n";
                }
                catch( exception& x )
                {
                    cout << "Fehler  " << x << "\n";
                }
            }

            if( write_end_of_line_to_stderr )  putc( '\n', stderr );
        }


        script_site.close();
    }
    catch( Script_exception& x )
    {
        cout << "Fehler  " << x << "\n";
    }
    catch( exception& x )
    {
        cerr << x << endl;
    }

    script_site.close();
    CoUninitialize();

    return 0;

USAGE:
    cerr << "usage: " << ( argv[0]? argv[0] : "javascript" ) << " -include=FILENAME -script=SCRIPT -sos.ini=FILENAME -interactive [FILENAME]\n";
    return 1;
}

//-------------------------------------------------------------------------------------------------
