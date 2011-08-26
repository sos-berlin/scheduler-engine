// $Id: anyfile2.h 11394 2005-04-03 08:30:29Z jz $

#ifndef __ANYFILE2_H
#define __ANYFILE2_H

#ifndef __SOSSTRNG_H
#   include "sosstrng.h"
#endif

#if !defined __ABSFILE_H
#   include "absfile.h"
#endif

#if !defined __SOSFIELD_H
#   include "sosfield.h"
#endif

#if !defined __ANYFILE_H
#   include "anyfile.h"
#endif

namespace sos
{

// Implementierung von Any_file

struct Any_file_param;

//---------------------------------------------------------------------------------Any_file_obj

struct Any_file_obj : Sos_object, File_base//: Abs_file
{
                                Any_file_obj            ();
                                Any_file_obj            ( const char* filename, Open_mode, const File_spec& = std_file_spec );
    virtual                    ~Any_file_obj            ();

    void                        open                    ( const char*, Open_mode );
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        open                    ( const Sos_string&, Open_mode );
    void                        open                    ( const Sos_string&, Open_mode, const File_spec& );
    void                        prepare_open            ( const Sos_string&, Open_mode, const File_spec& = std_file_spec );
    void                        filter_open             ( const Sos_string& filename, Open_mode, Any_file* );
    void                        clear_parameters        ();
    void                        bind_parameters         ( const Sos_ptr<Record_type>&, const Byte* );
    void                        set_parameter           ( int i/*1..n*/, const Dyn_obj& );
    void                        bind_parameter          ( int i/*1..n*/, Dyn_obj* );
    void                        open                    ();
    void                        close                   ( Close_mode close_mode = close_normal );
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
    void                        insert_key              ( const Const_area& area, const Key& );
    void                        store                   ( const Const_area& area );
    void                        store_key               ( const Const_area& area, const Key& );
    void                        update                  ( const Const_area& area );
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
    const File_spec&            spec                    ()                                  { return _spec; }
    File_info                   info                    ();

    static void                 default_type            ( const char* new_type );
    static const char*          default_type            ();

    File_spec                  _spec;   // nur die Dateitypen dürfen zugreifen!
    int4                        record_count            () const                                { return _record_count; }

            void                put                     ( const Const_area& area )                        { put_record( area ); }
            Bool                eof                     ();
            void                get                     ( Area* area_ptr )                                { get_record( *area_ptr ); }
            void                get                     ( Area* area_ptr, Record_lock record_lock )       { get_record_lock( *area_ptr, record_lock ); }
            void                get                     ( Area* area_ptr, const Key& key          )       { get_record_key( *area_ptr, key );         }
            void                get_key                 ( Area* area_ptr, const Key& key          )       { get_record_key( *area_ptr, key );         }
            void                get_until               ( Area* area_ptr, const Const_area& until_key );
/*obsolete*/void                get                     ( Area& area )                                    { get_record( area ); }
/*obsolete*/void                get                     ( Area& area, Record_lock record_lock )           { get_record_lock( area, record_lock ); }
/*obsolete*/void                get                     ( Area& area, const Key& key          )           { get_record_key( area, key );         }
            string              get_string              ();
    Dyn_obj                     get_dyn_obj             ();
    void                        get_position            ( Area*, const Const_area* until_key );

  protected:
    void                        put_record              ( const Const_area& area );
    void                        get_record              ( Area& area );
    void                        get_record_lock         ( Area& area, Record_lock lock );
    void                        get_record_key          ( Area& area, const Key& key );
    void                       _obj_print               ( ::std::ostream* ) const;

  private:
    friend struct               Any_file;

                                Any_file_obj            ( const Any_file_obj& );    // nicht implementiert

    void                        select_file_type        ( Any_file_param* );
    void                        callers_object_type     ( const Sos_ptr<Record_type>& );
    void                        callers_key_descr       ( const Sos_ptr<Field_descr>& );
    void                       _prepare_open            ( Any_file_param* );
    void                        prepare_open_preprocessing( Any_file_param* );
    void                        prepare_open_postprocessing( Any_file_param* );
    void                        modify_fields           ( Any_file_param* );
    void                        set_open_mode           ( int flag, Bool set );
    void                        build_key_type          ( Record_type* key_type, Record_type* record_type, uint offset );

  public:
    void                        new_file                ( Any_file* );          // Wenn der Dateityp beim open() Any_file_obj austauschen
    Sos_ptr<Abs_file>          _f;

//private:
    friend struct               Abs_file;

    Bool                       _opened;
    Bool                       _prepared;
    Open_mode                  _open_mode;

    Sos_static_ptr<Record_type> _record_type;
    Sos_static_ptr<Field_descr> _key_descr;
    Sos_string                 _type_name;
    Sos_string                 _filename;
    int                        _parameter_start;        // Start der Parameter in _filename
    Bool                       _prepare_open_not_implemented;
    int4                       _record_count;

    Bool                       _next_record_read;
    Dynamic_area               _record;
    Sos_ptr<Record_type>       _param_record_type;      // Für Any_file::set_parameter(int,Dyn_obj)
    Dynamic_area               _param_record;           // Für Any_file::set_parameter(int,Dyn_obj)

    const static char          _default_type[max_typename];
};

} //namespace sos

#endif

