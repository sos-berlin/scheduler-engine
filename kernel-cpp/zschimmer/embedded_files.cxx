// $Id: embedded_files.cxx 13839 2010-04-06 09:53:57Z ss $

#include "zschimmer.h"
#include "embedded_files.h"

/**
* \change 2.0.224 - jira-XXX: Dynamisch eingebundes XSD verwenden
* \detail
*/
#include "file.h"
#include <time.h>


using namespace std;


namespace zschimmer {

//-------------------------------------------------------------------------------------------static

static Message_code_text error_codes[] =
{
    { "Z-NO-EMBEDDED-FILE", "Datei $1 ist nicht eingebettet" },
    {}
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( embedded_files )
{
    add_message_code_texts( error_codes ); 
}

//--------------------------------------------------------------------------------get_embedded_file
    
const Embedded_file* Embedded_files::get_embedded_file( const string& filename ) const
{
    const Embedded_file* result = get_embedded_file_or_null( filename );
    if( !result )  throw_xc( "Z-NO-EMBEDDED-FILE", filename );
    return result;
}

//--------------------------------------------------------------------------------get_embedded_file
    
const Embedded_file* Embedded_files::get_embedded_file_or_null( const string& filename ) const
{
    
    for( const Embedded_file* f = _embedded_files; f->_filename; f++ )
    {
         if( f->_filename == filename )  return f;
    }

    return NULL;
}

//------------------------------------------------------------------------string_from_embedded_file

string Embedded_files::string_from_embedded_file( const string& filename ) const
{
    const Embedded_file* embedded_file = get_embedded_file( filename );
    return string( embedded_file->_content, embedded_file->_length );
}


/**
* \change 2.0.224 - JS-XXX: Liefert zu einem Key den Inhalt einer (Konfigurations-)datei.
* \detail
* Priorität haben bei Schedulerstart dynamisch geladen Dateien. 
* Wenn es zu dem Schlüssel kein dynamisch geladene Datei gibt, 
* dann wird das statische Dokument verwendet.
*/
string Embedded_and_dynamic_files::string_from_embedded_file(const string &filename)
{
	int size = _dynamic_files->size();
    // for( Embedded_file* f = _dynamic_files; f->_filename; f++ )
	for(int i = 0; i < size; ++i)
    {
		Dynamic_file* f = _dynamic_files->at(i);
		if(f != NULL) 
		{
			if( f->_filename == filename )  return string(f->_content->c_str(),f->_length);
		}
    }

	return _embedded_files.string_from_embedded_file(filename);
}


/**
* \change 2.0.224 - JS-XXX: File dynamisch laden
* \detail
* Lädt die Datei an dem angegebenen absoluten Pfad als string und macht den Inhalt für das Programm verfügbar.
*/
void Embedded_and_dynamic_files::set_dynamic_file (string& filename, const string key )
{
	if(_dynamic_files == NULL)
		_dynamic_files = new  vector<Dynamic_file*>;
	Dynamic_file* dynamic_file = new Dynamic_file();
	string* content = new string(file::string_from_file(filename));
	dynamic_file->_content = content;
	dynamic_file->_filename = key;
	dynamic_file->_last_modified_time = time(NULL);
//	string* start = content;
//	while(*content++);
//	int size = content - start;
	dynamic_file->_length = content->size();

	_dynamic_files->push_back(dynamic_file);
}



//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
