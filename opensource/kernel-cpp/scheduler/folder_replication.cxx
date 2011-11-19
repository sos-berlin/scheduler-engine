// $Id: folder_replication.cxx 13197 2007-12-06 12:05:42Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "../zschimmer/directory_lister.h"

namespace sos {
namespace scheduler {
namespace folder {
namespace replication {

using namespace zschimmer::file;

//-------------------------------------------------------------------------------------------------

/*
    Mit Supervisor verbinden

    <configuration.fetch_updated_files scheduler_id="..." signal_next_change_at_udp_port="...">
        <configuration.directory directory="/" timestamp="..">
            <configuration.file path="path" timestamp=".." md5="..."/>

            <configuration.directory directory="directory/" timestamp="..">
                <configuration.file path="path" timestamp=".." md5="..."/>
            </configuration.file>
        <configuration.directory>
    </configuration.fetch_updated_files>


    Antwort:

    <configuration.directory directory="/" timestamp="..">
        <configuration.file path="path" timestamp=".." md5="...">
            <content encoding="base64">...</content>
        </configuration.file>

        <configuration.file path="path" removed="yes"/>

        <configuration.directory directory="directory/" timestamp="..">
            <configuration.file path="path" timestamp="..">
                <content encoding="base64">...</content>
            </configuration.file>
        </configuration.file>
    </configuration.directory>


    UDP:

    <configuration.changed/>




    
    START

    Verbindung zum Supervisor aufbauen
    geht nicht?
        Normal starten
        Weiter versuchen, die Verbindung zum Supervisor aufzubauen

    Verbindung aufgebaut?
        Wenn noch nicht eigene Konfigurationsdateien geladen (state<active)
            Eigene Konfigurationsverzeichnisse lesen, Timestamp und MD5 einsammeln
        Sonst bereits geladene Informationen benutzen

        an Supervisor senden und dabei UDP-Signal für nächste Änderung bestellen


        Supervisor registriert Scheduler
        merkt sich am Scheduler gespeicherte Dateien (Zeitstempel, MD5)
        Startet Verzeichnisüberwachung (sowieso durch folder.cxx)
        vergleicht Dateien und liefert Änderungen



    Verzeichnisänderung am Supervisor:
        Für jeden registrierten Scheduler mit bestelltem UDP-Signal:
            Verzeichnisse durchgehen und Änderung feststellen
            Falls geändert, UDP-Signal
            UDP-Signal wiederholen, wenn Scheduler Dateien nicht abholt, aber seine TCP-Verbindung noch besteht


    ZEITSTEMPEL
        in UTC



    QUOTED-PRINTABLE, verändert für Scheduler "scheduler-printable"
        09, 0A, 20 bis 7C -> durchlassen
        00 bis 1F -> U+7E '~', U+7E '~', U+40 '@' bis U+5F      U+00 -> ~~@   U+0D -> ~~M
        80 bis 9E -> U+7E '~', U+7E '~', U+60 '`' bis U+7E      U+80 -> ~~`
        9F bis FC -> U+7E '~', U+20 bis U+7D                    U+9F -> ~~    U+FD -> ~~}
        FD bis FF -> U+7E '~', U+7E '~', U+30 '0' bis U+32 '2'  U+FD -> ~~0   U+FF -> ~~3
        XML: Erstes und letztes Blank codieren: "~ "


    ODER VERÄNDERTES UTF-8 (nein!)
        09, 0A, 20 bis 7E -> durchlassen
        00 bis 1F -> wie 100 bis 11F in UTF-8
        80 bis 9F -> wie 120 bis 140 in UTF-8
        A0 bis FF -> in UTF-8
            

*/

bool XXX::adjust_with_directory( const File_path& directory_path )
{
    file::Directory_lister dir ( _log );
    
    bool ok = dir.open( root_folder_path, directory_path );

    if( ok )
    {
        while( ptr<file::File_info> file_info = dir.get() )
        {
            string filename  = file_info->path().name();

            if( file_info->is_directory() )
            {
                xml_writer->begin_element( "configuration.directory" );
                xml_writer->set_attribute( "path"     , File_path( _directory, file_name ) );
                xml_writer->set_attribute( "timestamp", file_info->timestamp() );

                f( File_path( directory_path, filename ) );

                xml_writer->end_element( "configuration.directory" );
            }
            else
            {
                string name      = object_name_of_filename( filename );
                string extension = extension_of_filename( filename );
                
                if( name != "" )
                {
                    Typed_folder_map::iterator it = _typed_folder_map.find( extension );
                    if( it != _typed_folder_map.end() ) 
                    {
                        xml_writer->begin_element( "configuration.file" );
                        xml_writer->set_attribute( "path"     , File_path( _directory, file_name ) );
                        xml_writer->set_attribute( "timestamp", file_info->timestamp() );
                        xml_writer->set_attribute( "md5"      , md5( string_from_file( File_path( _directory, file_name ) ) ) );
                        xml_writer->end_element( "configuration.file" );
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------Subfolder_folder::on_base_file_changed

bool Subfolder_folder::on_base_file_changed( File_based* file_based, const Base_file_info* base_file_info, double now )
{
    bool    something_changed = false;
    Folder* subfolder         = static_cast<Folder*>( file_based );

    if( !subfolder )
    {
        ptr<Folder> new_subfolder = Z_NEW( Folder( subsystem(), folder() ) );
        new_subfolder->set_folder_path( folder()->path() );
        new_subfolder->set_name( base_file_info->_normalized_name );
        new_subfolder->fix_name();
        new_subfolder->set_base_file_info( *base_file_info );
        add_file_based( new_subfolder );
        something_changed = true;

        new_subfolder->activate();
        if( new_subfolder->file_based_state() >= File_based::s_loaded )  new_subfolder->adjust_with_directory( now );
    }
    else
    {
        subfolder->_file_is_removed = base_file_info == NULL;
        subfolder->_remove_xc       = zschimmer::Xc();

        if( base_file_info )
        {
            subfolder->set_base_file_info( *base_file_info );
            subfolder->set_to_be_removed( false ); 
            something_changed = subfolder->adjust_with_directory( now );    
        }
        else
        if( !subfolder->is_to_be_removed() ) 
        {
            string p = folder()->make_path( subfolder->base_file_info()._filename );
            subfolder->log()->info( message_string( "SCHEDULER-898" ) );

            subfolder->adjust_with_directory( now );
            subfolder->remove();

            something_changed = true;
        }
        else
        {
            // Verzeichnis ist gelöscht, aber es leben vielleicht noch Objekte, die gelöscht werden müssen.
            // adjust_with_directory() wird diese mit handle_replace_or_remove_candidates() löschen
            something_changed = subfolder->adjust_with_directory( now );    
        }
    }

    return something_changed;
}

//--------------------------------------------------------------Typed_folder::adjust_with_directory
    
bool Typed_folder::adjust_with_directory( const list<Base_file_info>& file_info_list, double now )
{
    bool                          something_changed  = false;
    vector<const Base_file_info*> ordered_file_infos;     // Geordnete Liste der vorgefundenen Dateien    
    vector<File_based*>           ordered_file_baseds;    // Geordnete Liste der bereits bekannten (geladenen) Dateien

    ordered_file_infos.reserve( file_info_list.size() );
    Z_FOR_EACH_CONST( list<Base_file_info>, file_info_list, it )  ordered_file_infos.push_back( &*it );
    sort( ordered_file_infos.begin(), ordered_file_infos.end(), Base_file_info::less_dereferenced );

    ordered_file_baseds.reserve( _file_based_map.size() );
    Z_FOR_EACH( File_based_map, _file_based_map, fb )  ordered_file_baseds.push_back( &*fb->second );
    sort( ordered_file_baseds.begin(), ordered_file_baseds.end(), File_based::less_dereferenced );


    vector<const Base_file_info*>::iterator fi = ordered_file_infos.begin();
    vector<File_based*          >::iterator fb = ordered_file_baseds.begin();      // Vorgefundene Dateien mit geladenenen Dateien abgleichen

    while( fi != ordered_file_infos.end()  ||
           fb != ordered_file_baseds.end() )
    {
        /// Dateinamen gleich Objektnamen?

        while( fb != ordered_file_baseds.end()  &&
               fi != ordered_file_infos.end()  &&
               (*fb)->_base_file_info._normalized_name == (*fi)->_normalized_name )
        {
            something_changed |= on_base_file_changed( *fb, *fi, now );
            fi++, fb++;
        }



        /// Dateien hinzugefügt?

        while( fi != ordered_file_infos.end()  &&
               ( fb == ordered_file_baseds.end()  ||  (*fi)->_normalized_name < (*fb)->_base_file_info._normalized_name ) )
        {
            something_changed |= on_base_file_changed( (File_based*)NULL, (*fi), now );
            fi++;
        }

        assert( fi == ordered_file_infos.end()  || 
                fb == ordered_file_baseds.end() ||
                (*fi)->_normalized_name >= (*fb)->_base_file_info._normalized_name );
        


        /// Dateien gelöscht?

        while( fb != ordered_file_baseds.end()  &&
               ( fi == ordered_file_infos.end()  ||  (*fi)->_normalized_name > (*fb)->_base_file_info._normalized_name ) )  // Datei entfernt?
        {
            something_changed |= on_base_file_changed( *fb, NULL, now );
            fb++;
        }

        assert( fb == ordered_file_baseds.end()  ||
                fi == ordered_file_infos.end()  ||
                (*fi)->_normalized_name <= (*fb)->_base_file_info._normalized_name );
    }

    return something_changed;
}

//-------------------------------------------------------------------------------------------------

} //namespace replication
} //namespace folder
} //namespace scheduler
} //namespace sos
