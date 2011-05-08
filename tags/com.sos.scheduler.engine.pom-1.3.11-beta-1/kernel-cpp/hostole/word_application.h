// $Id$

#ifndef __HOSTOLE_WORD_APPLICATION_H
#define __HOSTOLE_WORD_APPLICATION_H

#include "hostole2.h"

#undef ExitWindows

#if 0  // Word 2000
    //  #import "C:\Programme\Microsoft\Office\Office\MSO9.DLL"
    //  #import "C:\Programme\Gemeinsame Dateien\Microsoft Shared\VBA\VBA6\VBE6EXT.OLB"
#   import "MSO9.DLL"
    using namespace Office;

#   import "VBE6EXT.OLB"          // Include-Pfad muss VBA enthalten, z.B. C:\Programme\Gemeinsame Dateien\Microsoft Shared\VBA\VBA6
    using namespace VBIDE;

#   import "..\3rd_party\microsoft\msword9.olb"
    using namespace Word;

#else  // Word 97

    #ifdef RGB
        #undef RGB
    #endif

    #ifdef _CommandBars
        #undef _CommandBars
    #endif

#   import "..\3rd_party\microsoft\MSO97.DLL"
    namespace Office
    {
#       define _CommandBars CommandBars                      // ?? sonst nicht übersetzbar, jz 26.6.01
        typedef CommandBarsPtr _CommandBarsPtr;              // ?? sonst nicht übersetzbar, jz 26.6.01
    }
    using namespace Office;

#   import "..\3rd_party\microsoft\VBEEXT1.OLB"
    using namespace VBIDE;

#   import "..\3rd_party\microsoft\msword8.olb"
    using namespace Word;

#endif


namespace sos {

//---------------------------------------------------------------------------------Word_application

const GUID IID_Word_application;

struct Word_application : Iword_application, Sos_ole_object
{
    void*                       operator new                ( size_t size )                         { return sos_alloc( size, "hostWare.Word_application" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                                Word_application            ();
                               ~Word_application            ();

    USE_SOS_OLE_OBJECT

    STDMETHODIMP                Kill_all_words              ( BSTR, int* );
    STDMETHODIMP                Load                        ();
    STDMETHODIMP            get_App                         ( IDispatch** word_application_interface );
    STDMETHODIMP                Print                       ( BSTR filename, BSTR printer_name );

    HRESULT                     check_com_error             ( const _com_error& x, const char* method );
    void                        reset_printer_name          ();

  private:
                                Word_application            ( const Word_application& );            // nicht implementiert
    void                        operator =                  ( const Word_application& );            // nicht implementiert

    Fill_zero                  _zero_;
  //ptr<Office::Word::_Application> _app;
    _ApplicationPtr            _app;
  //Word_application*          _link;
    bool                       _active_printer_modified;
    string                     _original_active_printer;
    bool                       _original_active_printer_set;
    string                     _last_call;
};

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
