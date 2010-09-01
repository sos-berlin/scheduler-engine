// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_PATH_H
#define __SCHEDULER_PATH_H

namespace sos {
namespace scheduler {
namespace folder {

//-------------------------------------------------------------------------------------------------

struct Absolute_path;

//--------------------------------------------------------------------------------------------const

extern const Absolute_path      root_path;

//---------------------------------------------------------------------------------------------Path

struct Path : string
{
    static void                 self_test                   ();


                                Path                        ()                                      {}
                                Path                        ( const string& path )                  { set_path( path ); }
                                Path                        ( const char* path )                    { set_path( path ); }
                                Path                        ( const string& directory, const string& tail_path );

    Path&                       operator =                  ( const string& path )                  { set_path( path );  return *this; }
    Path&                       operator =                  ( const char* path )                    { set_path( path );  return *this; }


    void                    set_name                        ( const string& );                                
    string                      name                        () const;
    void                    set_folder_path                 ( const string& );
    Path                        folder_path                 () const;
    string                      root_folder_name            () const;
  //void                    set_absolute_if_relative        ( const Absolute_path& );
    void                    set_absolute                    ( const Absolute_path& absolute_base, const Path& relative );
  //void                        prepend_folder_path         ( const string& );
    const string&               to_string                   () const                                { return *static_cast<const string*>( this ); }
    void                    set_path                        ( const string& );
    bool                     is_absolute                    () const;
    string                      absolute_path               () const;
    bool                     is_root                        () const;
  //int                         depth                       () const;
    string                      to_filename                 () const;

  private:
    bool                        operator <                  ( const File_path& path ) const         { return compare( path ) <  0; }
    bool                        operator <=                 ( const File_path& path ) const         { return compare( path ) <= 0; }
    bool                        operator ==                 ( const File_path& path ) const         { return compare( path ) == 0; }
    bool                        operator !=                 ( const File_path& path ) const         { return compare( path ) != 0; }
    bool                        operator >=                 ( const File_path& path ) const         { return compare( path ) >= 0; }
    bool                        operator >                  ( const File_path& path ) const         { return compare( path ) >  0; }
    int                         compare                     ( const File_path& ) const;             // Nicht implementiert, weil Großkleinschreibung manchmal beachtet werden muss
};


inline void insert_into_message( Message_string* m, int index, const Path& path ) throw()           { return m->insert( index, path.to_string() ); }

//------------------------------------------------------------------------------------Absolute_path

struct Absolute_path : Path     //, javabridge::has_proxy<Absolute_path>
{
    static Absolute_path        build                       ( const File_based* source_file_based, const string& relative );
    static void                 self_test                   ();

                                Absolute_path               ()                                      {}
                              //Absolute_path               ( const string& path )                  { set_path( path ); }
                              //Absolute_path               ( const char* path )                    { set_path( path ); }
                                Absolute_path               ( const Absolute_path& absolute_directory, const string& path )  { set_absolute( absolute_directory, path ); }
                                Absolute_path               ( const Absolute_path& absolute_directory, const Bstr&   path )  { set_absolute( absolute_directory, string_from_bstr( path ) ); }
                                Absolute_path               ( const Absolute_path& absolute_directory, const char*   path )  { set_absolute( absolute_directory, path ); }
    explicit                    Absolute_path               ( const Path& );

    void                    set_path                        ( const string& );

    string                      with_slash                  () const;
    string                      without_slash               () const;
    Absolute_path               folder_path                 () const;

  private: 
    Absolute_path&              operator =                  ( const string& path );                 // Nicht implementiert
};

//-------------------------------------------------------------------------------------------------

} //namespace folder

using namespace folder;

} //namespace scheduler
} //namespace sos

#endif
