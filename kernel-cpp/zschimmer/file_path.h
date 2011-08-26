// $Id: file_path.h 13659 2008-09-11 06:35:58Z jz $

#ifndef __ZSCHIMMER_FILE_PATH_H
#define __ZSCHIMMER_FILE_PATH_H

#include "message.h"


struct stat;


namespace zschimmer {

struct Has_log;

namespace file {

//-------------------------------------------------------------------------------------------------

//std::list<string>               simplified_path_part_list   ( const std::list<string>& part_list );      // Kürzt "xx/.."
bool                            is_directory_separator      ( char );

//-----------------------------------------------------------------------------------------File_path

struct File_path : string
{
    static char                 directory_separator         ( const string& example_path = "" );
    static char                 directory_separator         ( const string& example_path1, const string& example_path2 );
    static void                 self_test                   ();


                                File_path                   ()                                      {}
                                File_path                   ( const string& path )                  { set_path( path ); }
                                File_path                   ( const string&    directory, const string& tail_path );

    File_path&                  operator =                  ( const string& path )                  { set_path( path );  return *this; }

    bool                        operator <                  ( const File_path& path ) const         { return compare( path ) <  0; }
    bool                        operator <=                 ( const File_path& path ) const         { return compare( path ) <= 0; }
    bool                        operator ==                 ( const File_path& path ) const         { return compare( path ) == 0; }
    bool                        operator !=                 ( const File_path& path ) const         { return compare( path ) != 0; }
    bool                        operator >=                 ( const File_path& path ) const         { return compare( path ) >= 0; }
    bool                        operator >                  ( const File_path& path ) const         { return compare( path ) >  0; }
    int                         compare                     ( const File_path& ) const;
    string                      normalized                  () const;

    void                    set_name                        ( const string& );                                
    string                      name                        () const;
    void                    set_directory                   ( const string& );
    File_path                   directory                   () const;
    void                        prepend_directory           ( const string& );
    string                      extension                   () const;
    string                      basename                    () const;
  //File_path                   simplified                  () const;
  //std::list<string>           part_list                   () const;                               // Die einzelnen Namen des Pfades
  //void                        assert_not_beyond_root      ( int current_depth = 0 );
    const string&               path                        ()  const                               { return *static_cast<const string*>( this ); }
    void                    set_path                        ( const string& path )                  { *static_cast<string*>( this ) = path; }
  //const string&               path                        () const                                { return _path; }
    string                      base_name                   () const;
    bool                     is_absolute_path               () const;
    bool                     is_relative_path               () const;
    string                      absolute_path               () const;
    void                        unlink                      () const;
    bool                        try_unlink                  ( Has_log* = NULL ) const;
    void                        move_to                     ( const File_path& ) const;
    void                        remove_complete_directory   () const;
    bool                        file_exists                 () const                                { return exists(); }    // Alternative Bedeutung: Verwiesene Datei (Inode) soll vorhanden sein
    bool                        exists                      () const;

  private:
    //string                     _path;
};

inline void                     insert_into_message         ( Message_string* m, int index, const File_path& f ) throw()  { return m->insert_string( index, f.path() ); }

//----------------------------------------------------------------------------------------File_info

struct File_info : Object
{
    static int                  quick_last_write_compare    ( const File_info*, const File_info* ); // last_write() muss vor einmal aufgerufen worden sein!
    static bool                 quick_last_write_less       ( const File_info*, const File_info* ); // last_write() muss vor einmal aufgerufen worden sein!

    
                                File_info                   ()                                      : _zero_( this+1 ) {}
    explicit                    File_info                   ( const File_path& );                   // ruft stat()

    const File_path&            path                        () const                                { return _file_path; }
    File_path&                  path                        ()                                      { return _file_path; }
    void                    set_path                        ( const File_path& path )               { _file_path = path; }

    void                    set_directory                   ( bool );
    bool                     is_directory                   ();
  //bool                     is_regular_file                ();

    void                    set_create_time                 ( time_t );
    time_t                      create_time                 ();
    void                    set_last_access_time            ( time_t );
    time_t                      last_access_time            ();
    void                    set_last_write_time             ( time_t );
    time_t                      last_write_time             ();

    bool                        try_call_stat               ();
    void                        call_stat                   ();
    void                        read_stat                   ( const struct stat& );
    void                        call_fstat                  ( int file_handle );


  private:
    void                        assert_uint32               ( time_t, const string& function );


    Fill_zero                  _zero_;
    
    // Speicher kompakt füllen (für 300.000 File_info's). 
    // Position 9
    char                       _create_time_filled          : 1;
    char                       _last_access_time_filled     : 1;
    char                       _last_write_time_filled      : 1;
    char                       _is_directory_filled         : 1;

    // Position 12
    uint32                     _create_time;                // uint32 gilt etwa bis zum Jahr 2106
    uint32                     _last_access_time;           // uint32 gilt etwa bis zum Jahr 2106
    uint32                     _last_write_time;            // uint32 gilt etwa bis zum Jahr 2106

    // Position 24
    File_path                  _file_path;

    // Position 52 (Visual C++ 2005)
    uint32                     _st_mode;

    // Position 56 (Visual C++ 2005)
};

//------------------------------------------------------------------------------------mini_string<>
#if 0

// Weitere Ersparnis mit File_info_ptr statt ptr<File_info>. Damit entfällt die vtbl. refcnt kann short sein. Oder Byte (mit assert).
// bool in drei Bits zusammenfassen: int _create_time_filled:1;
// Mit mini_string<27> wären das 32 Bytes statt jetzt 52
// Nur für einen Thread!
// Für eine handvoll weniger Pointer
// Auf Fill_zero verzichten (-1 Byte)
// Mit dieser Technik kann mini_string 5 Bytes kleiner werden: 1+1+4+4=10 oder (64bit) 1+1+8+4=14

struct simple_single_thread_object< class CLASS >
{
    AddRef() { return assert( _reference_count < BYTE_MAX ); _reference_count++; }
    Release() { if( --_reference_count == 0 )  delete this; }

    Byte    _reference_count;
}


// Speicherverwaltung für Objekte gleicher Größe braucht weniger Platz für die Verwaltung, bei großen Seiten
// Kann in Memory mapped files gehalten werden.

struct page< class OBJECT, uint PAGE_SIZE >
{
    OBJECT _object_array [ PAGE_SIZE / sizeof (OBJECT_SIZE) - ( ( ( PAGE_SIZE / sizeof (OBJECT) ) + 7 ) / 8 + sizeof (OBJECT) - 1 ) / sizeof (OBJECT) ) ];
    Byte [ ( ( PAGE_SIZE / sizeof (OBJECT) ) + 7 ) / 8 ] _bit_map;
};

struct Pages
{
    vector<Page*>           _pages;
};


//Etwas Sparsamer als File_path (28 Bytes brutto, 16 (-1?) Byte netto für internen String

template< uint SIZE >   // Empfehlung: SIZE >= sizeof (void*) + sizeof (int) + 1, 32bit: mini_string<9>, 64bit: mini_string<13>, besser aufgerundet: <16>, <24>
struct mini_string : simple_single_thread_object< mini_string< SIZE> >
{
    mini_string()
    {
        _chars[ SIZE ] = '\0';
        _chars[ 0 ] = '\0';
    }

  private:
    struct Allocated_string;
    {
        char*   _ptr;
        uint32  _size;
    };

    union
    {
        Allocated_string  _allocated_string;
        char              _chars            [ SIZE-1 ];     // Ein Zeichen weniger für _is_local_string, damit SIZE auf Wortgrenze liegen kann 
    };

    bool    _is_local_string;
};

typedef mini_string< 2 * sizeof (void*) + sizeof (uint32)>  Mini_string;

#endif
//-------------------------------------------------------------------------------------------------

} //namespace file
} //namespace zschimmer

#endif
