#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

using namespace directory_observer;

//-----------------------------------------------------------------------Typed_folder::Typed_folder

Typed_folder::Typed_folder( Folder* folder, Type_code type_code )
: 
    Scheduler_object( folder->_spooler, this, type_code ),
    _zero_(this+1),
    _folder(folder)
{
    log()->set_prefix( obj_name() );     // Noch ohne Pfad
}

//--------------------------------------------------------Typed_folder::remove_duplicates_from_list

void Typed_folder::remove_duplicates_from_list( list< const Directory_entry* >* directory_entry_list )
{
    typedef stdext::hash_map<string,const Directory_entry*>  Normalized_names_map ;
    Normalized_names_map                                     used_normalized_names;

    for( list< const Directory_entry* >::iterator de = directory_entry_list->begin(); de != directory_entry_list->end(); )
    {
        Normalized_names_map::iterator it = used_normalized_names.find( (*de)->_normalized_name );
        if( it != used_normalized_names.end() )
        {
            string duplicate_name = (*de)->_file_info->path().name();

          //if( !set_includes( _known_duplicate_filenames, duplicate_name ) )
            {
                log()->warn( message_string( "SCHEDULER-889", it->second->_file_info->path().name(), duplicate_name ) );      // Doppelte Datei

                //if( _spooler->_mail_on_error )
                //{
                //    Scheduler_event scheduler_event ( scheduler::evt_base_file_error, log_error, spooler() );
                //    scheduler_event.set_error( x );

                //    Mail_defaults mail_defaults( spooler() );
                //    mail_defaults.set( "subject", x.what() );
                //    mail_defaults.set( "body"   , x.what() );

                //    scheduler_event.send_mail( mail_defaults );
                //}

                //_known_duplicate_filenames.insert( duplicate_name );
                de = directory_entry_list->erase( de );
            }
        }
        else
        {
            used_normalized_names[ (*de)->_normalized_name ] = *de;
            de++;
        }
    }
}

//--------------------------------------------------------------Typed_folder::adjust_with_directory
    
bool Typed_folder::adjust_with_directory( const list<const Directory_entry*>& directory_entries )
{
    bool                           something_changed        = false;
    vector<const Directory_entry*> ordered_directory_entries;       // Geordnete Liste der Dateinamen
    vector<File_based*>            ordered_file_baseds;             // Geordnete Liste der bereits bekannten (geladenen) Dateien
                                                             

    ordered_directory_entries.reserve( _file_based_map.size() );
    Z_FOR_EACH_CONST( list<const Directory_entry*>, directory_entries, de )  ordered_directory_entries.push_back( *de );
    sort( ordered_directory_entries.begin(), ordered_directory_entries.end(), Directory_entry::normalized_less_dereferenced );

    ordered_file_baseds.reserve( _file_based_map.size() );
    Z_FOR_EACH( File_based_map, _file_based_map, fb )  ordered_file_baseds.push_back( &*fb->second );
    sort( ordered_file_baseds.begin(), ordered_file_baseds.end(), File_based::less_dereferenced );


    vector<const Directory_entry*>::const_iterator de = ordered_directory_entries.begin();
    vector<File_based*           >::const_iterator fb = ordered_file_baseds.begin();      // Vorgefundene Dateien mit geladenenen Dateien abgleichen

    while( de != ordered_directory_entries.end()  ||
           fb != ordered_file_baseds.end() )
    {
        /// Dateinamen gleich Objektnamen?

        while( fb != ordered_file_baseds.end()  &&
               de != ordered_directory_entries.end()  &&
               (*fb)->_base_file_info._normalized_name == (*de)->_normalized_name )
        {
            something_changed |= on_base_file_changed( *fb, *de );
            de++, fb++;
        }



        /// Dateien hinzugefügt?

        while( de != ordered_directory_entries.end()  &&
               ( fb == ordered_file_baseds.end()  ||  (*de)->_normalized_name < (*fb)->_base_file_info._normalized_name ) )
        {
            something_changed |= on_base_file_changed( (File_based*)NULL, *de );
            de++;
        }

        assert( de == ordered_directory_entries.end()  || 
                fb == ordered_file_baseds.end() ||
                (*de)->_normalized_name >= (*fb)->_base_file_info._normalized_name );
        


        /// Dateien gelöscht?

        while( fb != ordered_file_baseds.end()  &&
               ( de == ordered_directory_entries.end()  ||  (*de)->_normalized_name > (*fb)->_base_file_info._normalized_name ) )  // Datei entfernt?
        {
            something_changed |= on_base_file_changed( *fb, NULL );
            fb++;
        }

        assert( fb == ordered_file_baseds.end()  ||
                de == ordered_directory_entries.end()  ||
                (*de)->_normalized_name <= (*fb)->_base_file_info._normalized_name );
    }

    return something_changed;
}

//---------------------------------------------------------------Typed_folder::on_base_file_changed

bool Typed_folder::on_base_file_changed( File_based* old_file_based, const Directory_entry* directory_entry )
{
//#   ifdef Z_DEBUG
//        if( zschimmer::Log_ptr log = "zschimmer" )
//        {
//            log << Z_FUNCTION << "( ";
//            if( old_file_based )  log << old_file_based->obj_name() << " " << Time().set_utc( old_file_based->_base_file_info._last_write_time ).as_string()
//                                      << ( old_file_based->_file_is_removed? " file_is_removed" : "" );
//                            else  log << "new";
//            log << ", ";
//            if( directory_entry )  log << Time().set_utc( directory_entry->_file_info->last_write_time() ).as_string() << " " << directory_entry->_file_info->path();
//                             else  log << "removed file";
//            log << " )\n";
//        }
//#   endif

    //const Base_file_info* base_file_info = directory_entry->_file_info;
    bool            something_changed  = false;
    ptr<File_based> file_based         = NULL;
    File_based*     current_file_based = old_file_based;        // File_based der zuletzt gelesenen Datei
    bool            is_new             = !old_file_based  ||    
                                         old_file_based->_file_is_removed;     // Datei ist wieder aufgetaucht?
    File_path       file_path;

    assert( is_new || current_file_based );

    if( old_file_based )  
    {
        if( !old_file_based->_file_is_removed )  old_file_based->_remove_xc = zschimmer::Xc();      // Datei ist wieder da
        old_file_based->_file_is_removed = directory_entry == NULL;
        if( old_file_based->replacement() )  current_file_based = old_file_based->replacement();    // File_based der zuletzt geladenen Datei
    }


    try {
        if (directory_entry && !directory_entry->is_aging()) {
            string               name              = Folder::object_name_of_filename( directory_entry->_file_info->path().name() );
            ptr<file::File_info> changed_file_info = directory_entry->_file_info;
            bool                 file_is_different = false;
                
            if( !is_new ) {
                file_is_different = current_file_based->get_and_clear_force_file_reload() ||
                                    current_file_based->_base_file_info._last_write_time != directory_entry->_file_info->last_write_time()  ||
                                    current_file_based->name() != name;   // Objekt ist unter anderer Großschreibung bekannt?

                if( !file_is_different ) {
                    changed_file_info = current_file_based->changed_included_file_info();       // <include> geändert?
                    if( changed_file_info )  file_is_different = true;
                }
            }

            if( !is_new  &&  !file_is_different ) {
                ignore_duplicate_configuration_file( current_file_based, (File_based*)NULL, *directory_entry );
            } else {
                z::Xc content_xc;
                string source_xml;

                try {
                    source_xml = string_from_file( directory_entry->_file_info->path() );
                }
                catch( exception& x ) { content_xc = x; }

                if( content_xc.code() == ( S() << "ERRNO-" << ENOENT ).to_string() )    // ERRNO-2 (Datei gelöscht)?
                {
                    if( old_file_based ) {
                        old_file_based->_file_is_removed = true;
                        old_file_based->remove();
                        old_file_based = NULL;
                    }
                }
                else
                {   
                    something_changed = true;

                    file_based = subsystem()->call_new_file_based();
                    file_based->set_reloaded(!is_new);
                    file_based->set_file_based_state( File_based::s_undefined );    // Erst set_dom() definiert das Objekt
                    file_based->set_base_file_info( Base_file_info( *directory_entry ) );
                    file_based->set_folder_path( folder()->path() );
                    file_based->set_name( name );
                    file_based->fix_name();
                    file_based->_configuration_origin = directory_entry->_configuration_origin;

                    ignore_duplicate_configuration_file( current_file_based, file_based, *directory_entry );
                        
                    Time   t; 
                    t.set_utc( changed_file_info->last_write_time() );

                    if( old_file_based ) 
                    {
                        old_file_based->log()->info( message_string( "SCHEDULER-892", changed_file_info->path(), t.as_string(), subsystem()->object_type_name() ) );
                        old_file_based->handle_event( File_based::bfevt_modified ); 
                        old_file_based->set_replacement( file_based );
                        current_file_based = NULL;
                    }
                    else
                    {
                        file_based->log()->info( message_string( "SCHEDULER-891", changed_file_info->path(), t.as_string(), subsystem()->object_type_name() ) );
                        file_based->handle_event( File_based::bfevt_added );

                        add_file_based( file_based );
                    }


                    if( !content_xc.is_empty() )  throw content_xc;


                    xml::Document_ptr dom_document ( source_xml );
                    xml::Element_ptr  element      = dom_document.documentElement();
                    subsystem()->assert_xml_element_name( element );
                    if( spooler()->_validate_xml )  spooler()->_schema.validate( dom_document );

                    assert_empty_attribute( element, "spooler_id" );
                    if( !element.bool_getAttribute( "replace", true ) )  z::throw_xc( "SCHEDULER-232", element.nodeName(), "replace", element.getAttribute( "replace" ) );

                    Z_LOG2( "scheduler", file_path << ":\n" << source_xml << "\n" );

                    file_based->set_dom( element );         // Ruft clear_source_xml()
                    file_based->_source_xml = source_xml;   
                    file_based->set_file_based_state( File_based::s_not_initialized );

                    file_based->initialize();


                    if( file_based->file_based_state() == File_based::s_initialized )
                    {
                        if( !old_file_based )           // Neues Objekt?
                        {
                            file_based->activate();     
                        }
                        else
                        {
                            old_file_based->prepare_to_replace();

                            if( old_file_based->can_be_replaced_now() ) 
                            {
                                file_based = old_file_based->replace_now();     assert( !file_based->replacement() );
                            }
                        }
                    }
                }
            }
        }
        else                                        // Datei ist gelöscht
        if( old_file_based->has_base_file()  &&     // Nicht dateibasiertes Objekt, also aus anderer Quelle, nicht löschen
            !old_file_based->is_to_be_removed() )
        {
            something_changed = true;

            string p = folder()->make_path( old_file_based->base_file_info()._path );
            old_file_based->log()->info( message_string( "SCHEDULER-890", p, subsystem()->object_type_name() ) );

            file_based = old_file_based;                // Für catch()
            assert( file_based->_file_is_removed );
            
            file_based->handle_event( File_based::bfevt_removed );
            file_based->remove();
            file_based = NULL;
        }
    }
    catch( exception& x )
    {
        if( !file_based )  throw;   // Sollte nicht passieren

        string msg;


        if( directory_entry )        // Fehler beim Löschen soll das Objekt nicht als fehlerhaft markieren
        {
            file_based->_base_file_xc      = x;
            file_based->_base_file_xc_time = double_from_gmtime();

            Time t;
            t.set_utc( directory_entry->_file_info->last_write_time() );

            msg = message_string( "SCHEDULER-428", directory_entry->_file_info->path(), t.as_string(), x );
        }
        else
        {
            msg = message_string( "SCHEDULER-439", file_based->base_file_info()._path, 
                                                   file_based->subsystem()->object_type_name(), x );
        }

        file_based->log()->error( msg );

        if( msg != ""  &&  _spooler->_mail_on_error )
        {
            Scheduler_event scheduler_event ( scheduler::evt_base_file_error, log_error, spooler() );
            scheduler_event.set_error( x );

            Mail_defaults mail_defaults( spooler() );
            mail_defaults.set( "subject", msg );
            mail_defaults.set( "body"   , msg );

            scheduler_event.send_mail( mail_defaults );
        }
    }
    return something_changed;
}

//------------------------------------------------Typed_folder::ignore_duplicate_configuration_file
// Doppelte Dateinamen beim Mischen (cache und live) ignorieren

void Typed_folder::ignore_duplicate_configuration_file( File_based* current_file_based, File_based* new_file_based, const Directory_entry& directory_entry )
{
    //if( new_file_based  &&  current_file_based )  new_file_based->_duplicate_version = current_file_based->_duplicate_version;

    if( File_based* file_based = new_file_based? new_file_based : current_file_based )
    {
        if( directory_entry._duplicate_version  &&  
            directory_entry._duplicate_version != file_based->_duplicate_version  &&
            !directory_entry.is_aging() )
        {
            if( !new_file_based )
            {
                // Lokale Datei geändert, Objekt ist aber zentral definiert: 
                file_based->log()->warn( message_string( "SCHEDULER-460", subsystem()->object_type_name() ) );  // Geänderte lokale Datei wird ignoriert
            }
            else
            //if( current_file_based  &&  !current_file_based->_configuration_origin )
            //{
            //    file_based->log()->warn( message_string( "SCHEDULER-703" ) );   // Bereits gelesene lokale Datei wird durch zentrale ersetzt
            //}
            //else
            {
                file_based->log()->warn( message_string( "SCHEDULER-703" ) );   // Lokale Datei wird ignoriert und zentrale Version wird genommen
            }
        }

        if( current_file_based )  current_file_based->_duplicate_version = directory_entry._duplicate_version;
        if( new_file_based     )  new_file_based    ->_duplicate_version = directory_entry._duplicate_version;
    }
}

//-----------------------------------------------------Typed_folder::new_initialized_file_based_xml

ptr<File_based> Typed_folder::new_initialized_file_based_xml( const xml::Element_ptr& element, const string& default_name )
{
    subsystem()->check_file_based_element( element );
    //assert_empty_attribute( element, "replace"    );

    ptr<File_based> file_based = subsystem()->call_new_file_based();
    file_based->set_file_based_state( File_based::s_undefined );    // Erst set_dom() definiert das Objekt
    file_based->set_folder_path( folder()->path() );
    file_based->set_name( element.getAttribute( "name", default_name ) );
    file_based->set_dom( element );
    file_based->set_file_based_state( File_based::s_not_initialized );
    file_based->initialize();

    return file_based;
}

//-----------------------------------------------------------------Typed_folder::add_file_based_xml

void Typed_folder::add_file_based_xml( const xml::Element_ptr& element, const string& default_name )
{
    ptr<File_based> file_based = new_initialized_file_based_xml( element, default_name );
    add_file_based( file_based );
    file_based->activate();
}

//------------------------------------------------------Typed_folder::add_or_replace_file_based_xml

void Typed_folder::add_or_replace_file_based_xml( const xml::Element_ptr& element, const string& default_name )
{
    subsystem()->check_file_based_element( element );

    if( ptr<File_based> file_based = file_based_or_null( element.getAttribute( "name", default_name ) ) )
    {
        bool replace_yes        =  element.bool_getAttribute( "replace", false );                   // replace="yes"
        bool replace_no         = !element.bool_getAttribute( "replace", true  );                   // replace="no"
        bool use_base_mechanism = file_based_subsystem()->subsystem_state() <= subsys_initialized;  // Wird noch die Scheduler-Konfigurationsdatei geladen?

        //if( replace_no )  z::throw_xc( "SCHEDULER-441", obj_name() );   // replace="no" und Objekt ist bekannt

        if( replace_no  ||  
            use_base_mechanism  &&  !replace_yes )
        {
            file_based->set_dom( element );         // Objekt ergänzen (<base>) oder ändern. Evtl. Exception, wenn Objekt das nicht kann, z.B. <job>
        }
        else
        {
            ptr<File_based> replacement = new_initialized_file_based_xml( element );
            file_based->replace_with( replacement );
        }
    }
    else
    {
        //if( replace_no  && 
        //    file_based_subsystem()->subsystem_state() > subsys_initialized )   // Wird nicht die Scheduler-Konfigurationsdatei geladen? (Sonst <base> erlauben)
        //{
        //    z::throw_xc( SCHEDULER-441, obj_name() );   // replace="no" und Objekt ist nicht bekannt
        //}

        add_file_based_xml( element, default_name );
    }
}

//------------------------------------------------Typed_folder::add_to_replace_or_remove_candidates

void Typed_folder::add_to_replace_or_remove_candidates( const File_based& file_based )             
{ 
    _replace_or_remove_candidates_set.insert( file_based.name() ); 
    spooler()->folder_subsystem()->set_signaled( Z_FUNCTION );      // Könnte ein getrenntes Ereignis sein, denn das Verzeichnis muss nicht erneut gelesen werden.
}

//------------------------------------------------Typed_folder::handle_replace_or_remove_candidates

void Typed_folder::handle_replace_or_remove_candidates()
{
    if( !_replace_or_remove_candidates_set.empty() )
    {
        String_set my_path_set = _replace_or_remove_candidates_set;
        _replace_or_remove_candidates_set.clear();
        
        Z_FOR_EACH( String_set, my_path_set, it )
        {
            if( File_based* file_based = file_based_or_null( *it ) )
            {
                if( file_based->replacement()  &&  file_based->can_be_replaced_now() )
                {
                    file_based->log()->info( message_string( "SCHEDULER-936", subsystem()->object_type_name() ) );
                    file_based->replace_now();
                }
                else
                if( file_based->is_to_be_removed()  &&  file_based->can_be_removed_now() ) 
                {
                    file_based->log()->info( message_string( "SCHEDULER-937", subsystem()->object_type_name() ) );
                    file_based->remove_now();
                }
            }
        }
    } 
}

//---------------------------------------------------------------------Typed_folder::add_file_based

void Typed_folder::add_file_based( File_based* file_based )
{
    if( !file_based )  assert(0), z::throw_xc( Z_FUNCTION, "NULL" );
    assert( !file_based->typed_folder() );

    string normalized_name = file_based->normalized_name();
    if( normalized_name == ""  &&  !is_empty_name_allowed() )  z::throw_xc( "SCHEDULER-432", subsystem()->object_type_name() );

    if( file_based_or_null( normalized_name ) )  
        z::throw_xc( "SCHEDULER-160", subsystem()->object_type_name(), file_based->path().to_string() );

    file_based->fix_name();
    file_based->set_typed_folder( this );
    file_based->log()->set_prefix( file_based->obj_name() );        // Jetzt mit Pfad

    _file_based_map[ normalized_name ] = file_based;

    subsystem()->add_file_based( file_based );
}

//------------------------------------------------------------------Typed_folder::remove_file_based

void Typed_folder::remove_file_based( File_based* file_based )
{
    if( !file_based )  assert(0), z::throw_xc( Z_FUNCTION, "NULL" );
    assert( file_based->typed_folder() == this );
    assert( file_based->can_be_removed_now() );
    
    ptr<File_based> holder      = file_based;
    string          object_name = file_based->obj_name();

    _replace_or_remove_candidates_set.erase( file_based->path() );


    subsystem()->remove_file_based( file_based );


    File_based_map::iterator it = _file_based_map.find( file_based->normalized_name() );
    assert( it != _file_based_map.end() );
    if( it != _file_based_map.end() )
    {
        try
        {
            file_based->close();
        }
        catch( exception& x )
        {
            log()->warn( S() << x.what() << ", when removing " << file_based->obj_name() );
        }

        file_based->set_typed_folder( NULL );
        _file_based_map.erase( it );

        if( _file_based_map.empty() )  folder()->check_for_replacing_or_removing();
    }


    log()->log( subsystem()->subsystem_state() < subsys_stopped? log_info : log_debug9, message_string( "SCHEDULER-861", object_name ) );
}

//-----------------------------------------------------------------Typed_folder::replace_file_based

File_based* Typed_folder::replace_file_based( File_based* old_file_based )
{
    assert( old_file_based->replacement() );

    File_based* new_file_based  = old_file_based->replacement();
    string      normalized_name = new_file_based->normalized_name();

    if( old_file_based->normalized_name() != normalized_name )  assert(0), z::throw_xc( Z_FUNCTION );
    if( file_based( normalized_name ) != old_file_based )       assert(0), z::throw_xc( Z_FUNCTION );
    if( new_file_based->typed_folder() )                        assert(0), z::throw_xc( Z_FUNCTION );

    _replace_or_remove_candidates_set.erase( old_file_based->path() );

    old_file_based->set_typed_folder( NULL );
    new_file_based->set_typed_folder( this );
    _file_based_map[ normalized_name ] = new_file_based;

    subsystem()->replace_file_based( old_file_based, new_file_based );

    return new_file_based;
}

//-------------------------------------------------------------Typed_folder::remove_all_file_baseds

//void Typed_folder::remove_all_file_baseds()
//{
//    for( File_based_map::iterator it = _file_based_map.begin(); it != _file_based_map.end(); )
//    {
//        File_based_map::iterator next_it = it;  next_it++;
//
//        File_based* file_based = it->second;
//        if( !file_based->is_to_be_removed() )  file_based->remove();
//
//        it = next_it;
//    }
//}

//-------------------------------------------------------------------------Typed_folder::file_based

File_based* Typed_folder::file_based( const string& name ) const
{
    File_based* result = file_based_or_null( name );
    if( !result )  z::throw_xc( "SCHEDULER-430", file_based_subsystem()->object_type_name(), name );      // Sollte nicht passieren
    return result;
}

//-----------------------------------------------------------------Typed_folder::file_based_or_null

File_based* Typed_folder::file_based_or_null( const string& name ) const
{
    const File_based_map::const_iterator it = _file_based_map.find( subsystem()->normalized_name( name ) );
    return it == _file_based_map.end()? NULL 
                                      : it->second;
}

//----------------------------------------------------------------------------Typed_folder::set_dom

void Typed_folder::set_dom( const xml::Element_ptr& element )
{
    subsystem()->assert_xml_elements_name( element );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( !e.nodeName_is( subsystem()->xml_element_name() ) )  z::throw_xc( "SCHEDULER-409", subsystem()->xml_elements_name(), element.nodeName() );
        
        string spooler_id = element.getAttribute( "spooler_id" );

        if( spooler_id.empty()  ||  spooler_id == _spooler->id() )
        {
            add_or_replace_file_based_xml( e );
        }
    }
}

//------------------------------------------------------------------------Typed_folder::dom_element

xml::Element_ptr Typed_folder::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr result = new_dom_element( document, show_what );

    Z_FOR_EACH( File_based_map, _file_based_map, it )
    {
        File_based* file_based = it->second;
        
        if( file_based->is_visible_in_xml_folder( show_what ) )
        {
            result.appendChild( file_based->dom_element( document, show_what ) );
        }
    }

    return result;
}

//---------------------------------------------------------------------------Typed_folder::obj_name

string Typed_folder::obj_name() const
{
    S result;
    result << Scheduler_object::obj_name();

    if( _folder )
    {
        result << " " << _folder->path();
    }

    return result;
}

}}} //namespace sos::scheduler::folder
