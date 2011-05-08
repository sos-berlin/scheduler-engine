// anyfile.h

#ifndef __ANYFILE_H
#define __ANYFILE_H

#if !defined __FILEBASE_H
#   include "../file/filebase.h"
#endif

#if !defined __DYNOBJ
#   include "../kram/dynobj.h"
#endif

namespace sos
{

int4 copy_file( const char* source_name, const char* dest_name, long count = -1, long skip = 0 );
int4 copy_file( const Sos_string& source_name, const Sos_string& dest_name, long count = -1, long skip = 0 );
// Liefern die Anzahl der Sätze

//struct Any_base
//{
//   static void erase( const char* filename );
//   static void rename( const char* old_name, const char* new_name );
//};


void remove_file( const char* filename );
void rename_file( const char* old_filename, const char* new_filename );

inline void remove_file( const Sos_string& filename )                                      { remove_file( c_str( filename ) ); }
inline void rename_file( const Sos_string& old_filename, const Sos_string& new_filename )  { rename_file( c_str( old_filename ), c_str( new_filename ) ); }


struct Sos_object;
struct Any_file_obj;
struct Sos_database_session;


//-----------------------------------------------------------------------------------------Any_file

struct Any_file : Sos_self_deleting, File_base
{
                                Any_file                ();
                                Any_file                ( const char*   filename, Open_mode = (Open_mode)0, const File_spec& = std_file_spec );
                                Any_file                ( const string& filename, Open_mode = (Open_mode)0, const File_spec& = std_file_spec );
    virtual                    ~Any_file                ();

    void                        open                    ( const char*      , Open_mode = (Open_mode)0, const File_spec& = std_file_spec );
    void                        open                    ( const Sos_string&, Open_mode = (Open_mode)0, const File_spec& = std_file_spec );
    void                        prepare_open            ( const Sos_string&, Open_mode = (Open_mode)0, const File_spec& = std_file_spec );
    void                        prepare                 ( const Sos_string& n, Open_mode o = (Open_mode)0, const File_spec& s = std_file_spec ) { prepare_open( n, o, s ); }
    void                        clear_parameters        ();
    void                        bind_parameters         ( const Record_type*, const Byte* );
    void                        set_parameter           ( int i/*1..n*/, int );
    void                        set_parameter           ( int i/*1..n*/, const string& );
    void                        set_parameter           ( int i/*1..n*/, const Dyn_obj& );
    void                        bind_parameter          ( int i/*1..n*/, Dyn_obj* );
    void                        open                    ();
    void                        close                   ( Close_mode close_mode = close_normal );
    void                        destroy                 ();                                         // Löscht Any_file_obj
    void                        execute                 ();
/*
    static void                 erase                   ( const char* filename ) {
        Any_base::erase( filename );
    }

    static void                 rename                  ( const char* old_name, const char* new_name ) {
        Any_base::rename( old_name, new_name );
    }
*/
    void                        insert                  ( const Const_area& area );
    void                        insert                  ( const Record& record )                          { insert( record.const_area() ); }
    void                        insert_key              ( const Const_area& area, const Key& );
    void                        store                   ( const Const_area& area );
    void                        store                   ( const Record& record )                          { store( record.const_area() ); }
    void                        store_key               ( const Const_area& area, const Key& );
    void                        update                  ( const Const_area& area );
    void                        update                  ( const Record& record )                          { update( record.const_area() ); }
    void                        update_direct           ( const Const_area& area );
    void                        set                     ( const Key& key );
    void                        rewind                  ( Key::Number = 0 );
    void                        del                     ();
    void                        del                     ( const Key& key );
    void                        lock                    ( const Key& key, Record_lock = write_lock );

    Const_area                  current_key             ();
    Record_position             key_position            ( Key::Number = 0 );
    Record_length               key_length              ( Key::Number = 0 );
    Bool                        key_in_record           ();
    const File_spec&            spec                    ();
    File_info                   info                    ();
    string                      name                    () const;
    int4                        record_count            () const;
    Sos_ptr<Record_type>        record_type             ()                                                { return spec()._field_type_ptr; }

            void                put                     ( const Const_area& );
            void                put                     ( const string& str )                             { put( Const_area( str.c_str(), str.length() ) ); }
            void                put                     ( const char* str )                               { put( Const_area( str ) ); }
            void                put                     ( const Record& record )                          { put( record.const_area() ); }

            Bool                eof                     ();
    Record                      get_dyn_obj             ()                                                { return get_record(); }
    Record                      get_record              ();
            void                get                     ( Area* area_ptr );
            void                get                     ( Area* area_ptr, Record_lock );
            void                get                     ( Area* area_ptr, const Key& key          )       { get_key( area_ptr, key ); }
            void                get_key                 ( Area* area_ptr, const Key& key          );
            void                get_until               ( Area* area_ptr, const Const_area& until_key );
/*obsolete*/void                get                     ( Area& area )                                    { get( &area ); }
/*obsolete*/void                get                     ( Area& area, Record_lock record_lock )           { get( &area, record_lock ); }
/*obsolete*/void                get                     ( Area& area, const Key& key          )           { get_key( &area, key ); }
            string              get_string              ();                                                 
    void                        get_position            ( Area*, const Const_area* until_key = NULL );

    Record                      create_record           ();                                                 

    Bool                        opened                  ();   
    void                        obj_owner               ( Sos_object* );

    Sos_string                  identifier_quote_begin  ();
    Sos_string                  identifier_quote_end    ();
    Sos_string                  date_format             ();
    Sos_string                  date_time_format        ();
    string                      filename                ();
    Sos_database_session*       sos_database_session    ();
    Sos_database_session*       sos_database_session_or_null();
    Dbms_kind                   dbms_kind               ();
    string                      dbms_name               ();

    bool                        is_transaction_used     ();
    bool                        need_commit_or_rollback ();

  private:
    friend struct               Any_file_obj;
    void                        create                  ();

                              //Any_file                ( const Any_file& );    // nicht implementiert

    Sos_static_ptr<Any_file_obj> _file;
};

void init_file_types();

Sos_string file_as_string( const Sos_string& filename, const char* newline = "\n" );
Sos_string file_as_string( Any_file* opened_file, const char* newline = "\n" );

} //namespace sos

#endif

