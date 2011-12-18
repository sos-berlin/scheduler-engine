#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

//-------------------------------------------------------File_based_subsystem::File_based_subsystem

File_based_subsystem::File_based_subsystem( Spooler* spooler, IUnknown* iunknown, Type_code type_code ) 
: 
    Subsystem( spooler, iunknown, type_code ),
    _dependencies( this )
{
    spooler->_file_based_subsystems.insert( this );
}

//------------------------------------------------------File_based_subsystem::~File_based_subsystem
    
File_based_subsystem::~File_based_subsystem()
{
    _spooler->_file_based_subsystems.erase( this );
}

//------------------------------------------------------------File_based_subsystem::normalized_path
    
string File_based_subsystem::normalized_path( const Path& path ) const
{
    Path folder_path = path.folder_path();

    return Path( folder_path.is_root()  ||  folder_path.empty()? folder_path
                                                               : spooler()->folder_subsystem()->normalized_path( folder_path ), 
                 normalized_name( path.name() ) );
}

//----------------------------------------------------------------File_based_subsystem::dom_element

xml::Element_ptr File_based_subsystem::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what ) const
{
    xml::Element_ptr result = Subsystem::dom_element( dom_document, show_what );
    xml::Element_ptr statistics_element = dom_document.createElement( "file_based.statistics" );
    statistics_element.setAttribute( "count_visible", visible_file_based_count() );
    statistics_element.setAttribute( "count", file_based_count() );
    result.appendChild( statistics_element );
    return result;
}

//---------------------------------------------------File_based_subsystem::check_file_based_element

void File_based_subsystem::check_file_based_element( const xml::Element_ptr& element )
{
    assert_xml_element_name( element );

    if( element.getAttribute( "spooler_id" ) != ""  &&
        element.getAttribute( "spooler_id" ) != _spooler->id() )
    {
        log()->warn( message_string( "SCHEDULER-232", element.nodeName(), "spooler_id", element.getAttribute( "spooler_id" ) ) );
    }
}

//---------------------------------------------------------------File_based_subsystem::typed_folder

Typed_folder* File_based_subsystem::typed_folder( const Absolute_path& path ) const     
{
    return _spooler->folder_subsystem()->folder( path )->typed_folder( this ); 
}

//----------------------------------------------------File_based_subsystem::assert_xml_element_name

void File_based_subsystem::assert_xml_element_name( const xml::Element_ptr& e ) const
{ 
    if( !e.nodeName_is( xml_element_name() ) )  z::throw_xc( "SCHEDULER-409", xml_element_name(), e.nodeName() );
}

//---------------------------------------------------File_based_subsystem::assert_xml_elements_name

void File_based_subsystem::assert_xml_elements_name( const xml::Element_ptr& e ) const
{ 
    if( !e.nodeName_is( xml_elements_name() ) )  z::throw_xc( "SCHEDULER-409", xml_elements_name(), e.nodeName() );
}

//---------------------------------------------------File_based_subsystem::name_from_xml_attributes

string File_based_subsystem::name_from_xml_attributes( const xml::Element_ptr& element, Handle_attributes handle_attributes ) const
{
    string         name;
    vector<string> name_attributes = vector_split( " ", this->name_attributes() );

    for( int i = 0; i < name_attributes.size(); i++ )  
    {
        if( i > 0 )  name += folder_name_separator;
        name += element.getAttribute_mandatory( name_attributes[ i ] );     // name=  oder, f�r Order, job_chain= und id= durch Komma getrennt

        if( handle_attributes == remove_attributes )  element.removeAttribute( name_attributes[ i ] );
    }

    return name;
}

//----------------------------------------------------------------File_based_subsystem::execute_xml

xml::Element_ptr File_based_subsystem::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& show_what )
{
    xml::Element_ptr result;

    if( element.nodeName_is( xml_element_name() ) )
    {
        typed_folder( root_path )->add_or_replace_file_based_xml( element );
        result = command_processor->_answer.createElement( "ok" );
    }
    else
    if( string_begins_with( element.nodeName(), xml_element_name() + "." ) ) 
    {
        result = file_based( Absolute_path( root_path, element.getAttribute( xml_element_name() ) ) )->execute_xml( command_processor, element, show_what );
    }
    else
        z::throw_xc( "SCHEDULER-113", element.nodeName() );

    return result;
}

}}} //namespace sos::scheduler::folder
