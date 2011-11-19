// tabbed.h

#ifndef __TABBED_H
#define __TABBED_H

#ifndef __SOSARRAY_H
#include <sosarray.h>
#endif

#ifndef __SOSFIELD_H
#include <sosfield.h>
#endif

namespace sos {

struct Tabbed_file : Abs_file
{
    ~Tabbed_file();
    Tabbed_file();
    Tabbed_file( const char* szdatasource, Open_mode open_mode, const File_spec& file_spec );

    void open( const char* filename, Open_mode open_mode, const File_spec& file_spec );
    void close( Close_mode close_mode );

    void                            insert( const Const_area& area );
    void                            update( const Const_area& area );

    void                            set( const Key& key );
    void                            del( const Key& key );
    void                            del();

    Record_position                 key_position (Key::Number = 0) { return _key_position; };
    Record_length                   key_length   (Key::Number = 0) { return _key_length; };

    static const Abs_file_type&     static_file_type();

  private:
    void                            _init();
    void                            _convert( const Const_area& area, Area* area_ptr );
    uint                            _lookup_named_field( const char* field_name );
    void                            _init_field_order();
    void                            _insert_field_order();

    void                            _replace_select_all( Sos_string* );
    void                            print_field_names( std::ostream* s, const Record_type& record_type, const char* sep = ", ", const char* = "" );

    Any_file                        _file;
    Dynamic_area                    _current_key;
    Open_mode                       _open_mode;

    Record_position                 _key_position;
    Record_position                 _key_length;

    Sos_ptr<Record_type>            _record_type_ptr;
    Sos_ptr<Field_descr>            _key_descr_ptr;
    Text_format                     _format;

    Bool                            _field_names;
    Bool                            _with_field_order;
    Bool                            _transparent_write;
    Sos_simple_array<int>           _field_order;
  protected:

    void                            get_record( Area& area );
    void                            get_record_key( Area& area, const Key& key );

    void                            put_record( const Const_area& area ) { insert( area ); }

};

} //namespace sos

#endif


