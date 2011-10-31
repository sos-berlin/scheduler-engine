#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

using namespace directory_observer;

//---------------------------------------------------------------Subfolder_folder::Subfolder_folder

Subfolder_folder::Subfolder_folder( Folder* folder )
:
    typed_folder<Folder>( folder->spooler()->folder_subsystem(), folder, type_subfolder_folder )
{
}

//--------------------------------------------------------------Subfolder_folder::~Subfolder_folder
    
Subfolder_folder::~Subfolder_folder()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG( Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//-----------------------------------------------------------Subfolder_folder::on_base_file_changed

bool Subfolder_folder::on_base_file_changed( File_based* file_based, const Directory_entry* directory_entry )
{
    bool    something_changed = false;
    Folder* subfolder         = static_cast<Folder*>( file_based );

    if( !subfolder )
    {
        ptr<Folder> new_subfolder = Z_NEW( Folder( subsystem(), folder() ) );
        new_subfolder->set_folder_path( folder()->path() );
        new_subfolder->set_name( directory_entry->_normalized_name );
        new_subfolder->fix_name();
        new_subfolder->set_base_file_info( Base_file_info( *directory_entry ) ); 
        add_file_based( new_subfolder );                    
        something_changed = true;

        new_subfolder->activate();
        if( new_subfolder->file_based_state() >= File_based::s_loaded )  new_subfolder->adjust_with_directory( directory_entry->_subdirectory );
    }
    else
    {
        subfolder->_file_is_removed = directory_entry == NULL;
        subfolder->_remove_xc       = zschimmer::Xc();

        if( directory_entry )
        {
            Z_DEBUG_ONLY( assert( directory_entry->_subdirectory ) );

            subfolder->set_base_file_info( Base_file_info( *directory_entry ) );
            subfolder->set_to_be_removed( false ); 
            something_changed = subfolder->adjust_with_directory( directory_entry->_subdirectory );    
        }
        else
        if( !subfolder->is_to_be_removed() ) 
        {
            string p = folder()->make_path( subfolder->base_file_info()._filename );
            subfolder->log()->info( message_string( "SCHEDULER-898" ) );
            subfolder->adjust_with_directory( NULL );
            subfolder->remove();

            something_changed = true;
        }
        else
        {
            // Verzeichnis ist gelöscht, aber es leben vielleicht noch Objekte, die gelöscht werden müssen.
            // adjust_with_directory() wird diese mit handle_replace_or_remove_candidates() löschen
            something_changed = subfolder->adjust_with_directory( NULL );    
        }
    }

    return something_changed;
}

}}} //namespace sos::scheduler::folder
