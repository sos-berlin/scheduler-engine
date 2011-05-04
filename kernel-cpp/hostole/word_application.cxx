// $Id$          Joacim Zschimmer

#include "precomp.h"

#include <set>
#include <windows.h>
#include <tlhelp32.h>

#include "../kram/sos.h"
#include "../kram/olestd.h"
#include "../kram/thread_semaphore.h"

#include "hostole2.h"
#include "word_application.h"

//-------------------------------------------------------------------------------------------------

namespace sos {

const int word_ist_weg_fehler = 0x800706BA;

//------------------------------------------------------------------------------------Typbibliothek

DESCRIBE_CLASS_CREATABLE( &hostole_typelib, Word_application, word_application, CLSID_Word_application, "hostWare.Word_application", "1.0" );

//static Word_application* app_list;

//----------------------------------------------------------------------terminate_word_applications
/*
void terminate_word_applications()
{
    Word_application* a = app_list;
    while( a )
    {
        Word_application* b = a->_link;
        try { delete a; }
        catch( const _com_error& ) {}
        a = b;
    }
}
*/
//---------------------------------------------------------------Word_application::Word_application

Word_application::Word_application()
:
    Sos_ole_object( &word_application_class, this ),
    _zero_(this+1)
{
/*
    _link = app_list;
    app_list = this;
*/
}

//--------------------------------------------------------------Word_application::~Word_application

Word_application::~Word_application()
{
    if( _app != NULL )
    {
        reset_printer_name();

        try
        {
            LOG( "Word.Application.Quit\n" );
            _last_call = "Word.Application.Quit()";

            _app->Quit();
        }
        catch( const _com_error& x )  { LOG( "Fehler bei Word.Application::Quit: " << x << '\n' ); }
    }

/*
    if( app_list == this )
    {
        app_list = _link;
    }
    else
    {
        Word_application* a = app_list;
        while( a  &&  a->_link != this )  a = a->_link;
        if( a )  a->_link = _link;
    }

    _link = NULL;   
*/
}

//-------------------------------------------------------------Word_application::reset_printer_name

void Word_application::reset_printer_name()
{
    try
    {
        if( _active_printer_modified )  
        {
            LOG( "Word.Application reset ActivePrinter=\"" << _original_active_printer << "\"\n" );
            _last_call = "Word.Application.ActivePrinter=\"" + _original_active_printer + "\"";

            _app->ActivePrinter = _original_active_printer.c_str();

            _active_printer_modified = false;
        }
    }
    catch( const _com_error& x )  { LOG( "Fehler bei Word.Application::ActivePrinter: " << x << '\n' ); }
}

//----------------------------------------------------------------Word_application::check_com_error

HRESULT Word_application::check_com_error( const _com_error& x, const char* method )
{
    HRESULT hr = _set_excepinfo( x, method );

    if( hr == word_ist_weg_fehler ) 
    {
        LOG( "Der Verweis auf Word wird gelöscht\n" );
        _app = NULL;
    }

    return hr;
}

//-------------------------------------------------------------------------------------kill_process
/*

static bool kill_process( const PROCESSENTRY32& process )
{
    bool result = false;

    LOG( "kill " << e.th32ProcessID << " " << e.szExeFile << '\n' );

    HANDLE h = OpenProcess( PROCESS_TERMINATE, FALSE, e.th32ProcessID );
    if( h )
    {
        // Prüfen, ob die Process Id noch demselben Programm gehört
        HANDLE          snapshot = createsnapshot( TH32CS_SNAPPROCESS, 0 );
        PROCESSENTRY32  e;         memset( &e, 0, sizeof e );  e.dwSize = sizeof e;
        
        BOOL ok = process32first( snapshot, &e );

        while( ok ) 
        {
            if( e.th32ParentProcessID == process.th32ParentProcessID  &&  strcmp( e.szExeFile, process.szExeFile ) == 0 )
            {
                TerminateProcess( h, (uint)-1 );
                CloseHandle( h );

                result = true;
            }

            ok = process32next( snapshot, &e );
        }

        closesnapshot( snapshot );
    }
    else
    {
        LOG( "  " << get_mswin_msg_text( GetLastError() ) << '\n' );
    }

    return result;
}

*/
//-----------------------------------------------------------------Word_application::kill_all_words

STDMETHODIMP Word_application::Kill_all_words( BSTR window_caption, int* count )
{
    HRESULT hr = NOERROR;

    *count = 0;

    try
    {
        LOGI( "Word_application::kill_all_words\n" );

        OSVERSIONINFO v;  memset( &v, 0, sizeof v ); v.dwOSVersionInfoSize = sizeof v;
        GetVersionEx( &v );
        if( v.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS  ||  v.dwMajorVersion >= 5 )     // Windows >= 95  oder Windows >= 2000? (Nicht NT bis 4.0)
        {
            LOG( "LoadLibrary( \"kernel32.dll\" )\n" );

            HINSTANCE lib = LoadLibrary( "kernel32.dll" );
            if( lib )
            {
                typedef HANDLE (WINAPI *Proc1)(DWORD,DWORD); 
                typedef BOOL   (WINAPI *Proc )(HANDLE,LPPROCESSENTRY32);

                Proc1 createsnapshot = (Proc1)GetProcAddress( lib, "CreateToolhelp32Snapshot" );
                Proc  process32first = (Proc) GetProcAddress( lib, "Process32First" );
                Proc  process32next  = (Proc) GetProcAddress( lib, "Process32Next" );

                if( createsnapshot && process32first && process32next )
                {
                    LOG( "createsnapshot()\n" );
                    HANDLE          snapshot = createsnapshot( TH32CS_SNAPPROCESS, 0 );
                    PROCESSENTRY32  e;
                    std::set<DWORD> svchost_set;        // Processkennungen von svchost.exe, dem Parent von EXE-COM-Servern (so bis Windows 2000).

                    memset( &e, 0, sizeof e );
                    e.dwSize = sizeof e;

                    LOG( "process32first()\n" );

                    BOOL ok = process32first( snapshot, &e );

                    while(1) 
                    {
                        string filename = basename_of_path( e.szExeFile ) + "." + extension_of_path( e.szExeFile );
                        if( stricmp( filename.c_str(), "svchost.exe" ) == 0 )  svchost_set.insert( e.th32ProcessID );

                        ok = process32next( snapshot, &e );
                        if( !ok )  break;
                    }

                    CloseHandle( snapshot );

                    if( svchost_set.empty() )
                    {
                        LOG( "Der Prozess svchost.exe fehlt. Kein Word wird beendet.\n" );
                        goto ENDE;
                    }



                    LOG( "createsnapshot()\n" );
                    snapshot = createsnapshot( TH32CS_SNAPPROCESS, 0 );

                    memset( &e, 0, sizeof e );
                    e.dwSize = sizeof e;

                    LOG( "process32first()\n" );
                    ok = process32first( snapshot, &e );

                    while(1) 
                    {
                        string filename = basename_of_path( e.szExeFile ) + "." + extension_of_path( e.szExeFile );
                        if( stricmp( filename.c_str(), "winword.exe" ) == 0  &&  svchost_set.find(e.th32ParentProcessID) != svchost_set.end() )
                        {
                            LOG( "kill " << e.th32ProcessID << " " << e.szExeFile << '\n' );

                            HANDLE h = OpenProcess( PROCESS_TERMINATE, FALSE, e.th32ProcessID );
                            if( h )
                            {
                                BOOL ok = TerminateProcess( h, (uint)-1 );
                                if( !ok ) 
                                {
                                    try { throw_mswin_error( "TerminateProcess", "winword.exe" ); }
                                    catch( const exception& ) {}
                                }
                                (*count)++;
                                CloseHandle( h );
                            }
                            else
                            {
                                LOG( "  " << get_mswin_msg_text( GetLastError() ) << '\n' );
                            }
                        }

                        ok = process32next( snapshot, &e );
                        if( !ok )  break;
                    }

                    CloseHandle( snapshot );
                }

ENDE:
                LOG( "FreeLibrary()\n" );
                FreeLibrary( lib );
            }
        }

        LOG( "Word_application::kill_all_words OK\n" );
    }
    catch( const Xc&         x )  { hr = _set_excepinfo( x, "hostWare.Word_application::kill_all_words" ); }
    catch( const xmsg&       x )  { hr = _set_excepinfo( x, "hostWare.Word_application::kill_all_words" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Word_application::kill_all_words" ); }

    return hr;
}

//---------------------------------------------------------------------------Word_application::load

STDMETHODIMP Word_application::Load()
{
    HRESULT hr = NOERROR;

    if( _app )  return hr;

    LOGI( "Word_application::load Word.Application.8\n" );

    try
    {
        hr = _app.CreateInstance( L"Word.Application.8" );   // Schnittstelle von Word 97

        LOG( "version=" <<_app->Version << " build=" << _app->Build << '\n' );
    }
    catch( const Xc&         x )  { hr = _set_excepinfo ( x, "hostWare.Word_application::load" ); }
    catch( const xmsg&       x )  { hr = _set_excepinfo ( x, "hostWare.Word_application::load" ); }
    catch( const _com_error& x )  { hr = check_com_error( x, "hostWare.Word_application::load" ); }

    return hr;
}

//---------------------------------------------------------------------------Word_application::load

STDMETHODIMP Word_application::get_App( IDispatch** word_application_interface )
{
    HRESULT hr = NOERROR;

    if( _app == NULL )  
    {
        hr = Load();
        if( FAILED(hr) )  return hr;
    }

    _app->QueryInterface( IID_IDispatch, (void**)word_application_interface );

    return hr;
}

//--------------------------------------------------------------------------Word_application::print

STDMETHODIMP Word_application::Print( BSTR filename_bstr, BSTR param_bstr )
{
    HRESULT hr       = NOERROR;
    HRESULT close_hr = NOERROR;

    try
    {
        LOGI( "Word_application::print " << filename_bstr << ", " << param_bstr << '\n' );
        
        _last_call = "";

        string  output_file;
        string  printer_name;
        string  filename = make_absolute_filename( ".", bstr_as_string( filename_bstr ) );
        Variant copies   = 1;
        Variant collate  = Variant::vt_missing;
        Variant pages    = Variant::vt_missing;

        for( Sos_option_iterator o = bstr_as_string( param_bstr ); !o.end(); o.next() )
        {
            if( o.with_value( "file"            ) )  output_file = make_absolute_filename( ".", o.value() );
            else
            if( o.with_value( "printer"         ) )  printer_name = o.value();
            else
            if( o.with_value( "copies"          ) )  copies  = o.value();
            else
            if( o.flag      ( "collate"         ) )  collate = o.set();
            else
            if( o.with_value( "pages"           ) )  pages   = o.value();
            else
                throw_sos_option_error(o);
        }

        if( _app == NULL )  { hr = Load(); if( FAILED(hr) )  return hr; }

        if( copies != Variant::vt_missing  &&  collate == Variant::vt_missing )  collate = true;     // Words Default ist false, das ändern wir. Druckreihenfolge soll sein 1,2,3,..,1,2,3,...

        if( !printer_name.empty() )  
        {
            if( !_original_active_printer_set )  
            {
                _last_call = "Lese Word.Application.ActivePrinter";
                _original_active_printer = bstr_as_string( _app->ActivePrinter );
                _original_active_printer_set = true;
                LOG( "Word.Application.ActivePrinter == \"" + _original_active_printer  + "\"\n" );
            }

            if( stricmp( _original_active_printer.c_str(), printer_name.c_str() ) != 0 )
            {
                LOG( "ActivePrinter=\"" << printer_name << "\"\n" );
                _last_call = "Word.Application.ActivePrinter=\"" + printer_name + "\"";

                _app->ActivePrinter = printer_name.c_str();

                _active_printer_modified = true;
            }
        }



        LOG( "Open " << filename << '\n' );
        _last_call = "Word.Documents.Open(\"" + filename + "\")";

        _DocumentPtr doc = _app->Documents->Open( 
            &Variant( filename.c_str() ),       // VARIANT* FileName,
            &Variant( false ),                  // VARIANT* ConfirmConversions = &vtMissing,
            &Variant( true ),                   // VARIANT* ReadOnly = &vtMissing,
            &Variant( false ),                  // VARIANT* AddToRecentFiles = &vtMissing,
            &vtMissing,                         // VARIANT* PasswordDocument = &vtMissing,
            &vtMissing,                         // VARIANT* PasswordTemplate = &vtMissing,
            &vtMissing,                         // VARIANT* Revert = &vtMissing,
            &vtMissing,                         // VARIANT* WritePasswordDocument = &vtMissing,
            &vtMissing,                         // VARIANT* WritePasswordTemplate = &vtMissing,
            &vtMissing                          // VARIANT* Format = &vtMissing,

    //Word9 &vtMissing,                         // VARIANT* Encoding = &vtMissing,
    //Word9 &Variant( true )                    // VARIANT* Visible = &vtMissing 
        );



        Variant output_file_vt = output_file.c_str(); 
        Variant range          = pages == Variant::vt_missing? wdPrintAllDocument : wdPrintRangeOfPages;

        LOG( "PrintOut\n" );
        _last_call = "Word.Document.PrintOut()";

        hr = doc->PrintOut(
            &Variant( false ),                  // VARIANT* Background = &vtMissing,
            &Variant( false ),                  // VARIANT* Append = &vtMissing,
            &range,                             // VARIANT* Range = &vtMissing,
            &output_file_vt,                    // VARIANT* OutputFileName = &vtMissing,
            &vtMissing,                         // VARIANT* From = &vtMissing,
            &vtMissing,                         // VARIANT* To = &vtMissing,
            &vtMissing,                         // VARIANT* Item = &vtMissing,
            &copies,                            // VARIANT* Copies = &vtMissing,
            &pages,                             // VARIANT* Pages = &vtMissing,
            &vtMissing,                         // VARIANT* PageType = &vtMissing,
            &Variant( !output_file.empty() ),   // VARIANT* PrintToFile = &vtMissing,
            &collate,                           // VARIANT* Collate = &vtMissing,
            &vtMissing,                         // VARIANT* ActivePrinterMacGX = &vtMissing,
            &vtMissing                          // VARIANT* ManualDuplexPrint = &vtMissing,

    //Word9 &vtMissing,                         // VARIANT* PrintZoomColumn = &vtMissing,
    //Word9 &vtMissing,                         // VARIANT* PrintZoomRow = &vtMissing,
    //Word9 &vtMissing,                         // VARIANT* PrintZoomPaperWidth = &vtMissing,
    //Word9 &vtMissing                          // VARIANT* PrintZoomPaperHeight = &vtMissing 
        );

        _last_call = "Word.Document.Close()";
        close_hr = doc->Close( &Variant( wdDoNotSaveChanges ) );

        //reset_printer_name();

        if( !FAILED(hr) )  hr = close_hr;
    }
    catch( const Xc&         x )  { hr = _set_excepinfo ( x, ( "hostWare.Word_application::print, " + _last_call ).c_str() ); }
    catch( const xmsg&       x )  { hr = _set_excepinfo ( x, ( "hostWare.Word_application::print, " + _last_call ).c_str() ); }
    catch( const _com_error& x )  { hr = check_com_error( x, ( "hostWare.Word_application::print, " + _last_call ).c_str() ); }

    _last_call = "";

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
