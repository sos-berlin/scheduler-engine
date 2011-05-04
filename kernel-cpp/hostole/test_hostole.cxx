// $Id$

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hostole.h"

int main( int, char* )
{
    Ihostware_file* file = NULL;
    HRESULT hr;

/*
    Bstr filename = "-log -out /tmp/hostole";
    Bstr line     = "Hier ist test_hostole.cxx";

    fprintf( stderr, "CoCreateInstance\n" );
    hr = CoCreateInstance( CLSID_File, NULL, 0, IID_Ihostware_file, (void**)&file );
    if( FAILED(hr) )  goto FEHLER;

    fprintf( stderr, "open\n" );
    hr = file->open( filename );
    if( FAILED(hr) )  goto FEHLER;

    fprintf( stderr, "put_line\n" );
    hr = file->put_line( line );
    if( FAILED(hr) )  goto FEHLER;

    hr = file->close();

    file->Release();
*/

    ptr<Ivariable> v;
    ptr vt;
    ptr<Ifactory_processor> processor;
    processor.CoCreateInstance( CLSID_Factory_processor );

    hr = processor->put_language( Bstr("PerlScript") );   if( FAILED(hr) )  goto FEHLER;
    hr = processor->put_template_filename( Bstr("/home/joacim/tmp/template.txt") );
    hr = processor->put_document_filename( Bstr("/home/joacim/tmp/doc.txt") );
    hr = processor->put_parameter( Bstr("x"), &Variant("HEUTE") );
    hr = processor->get_parameter( Bstr("x"), &v );
    hr = processor->eval( Bstr("1"), (Scripttext_flags)0, &vt );
    hr = processor->put_parameter( Bstr("brief_datum"), &Variant("27.3.2002") );
    hr = processor->parse( Bstr("$a=\"A\""), (Scripttext_flags)0, &vt );
    hr = processor->process();

    return 0;

FEHLER:
    fprintf( stderr, "hr=%X\n", (int)hr );
}
