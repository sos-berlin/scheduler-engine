// sqlfile.h

#ifndef __SQLFILE_H
#define __SQLFILE_H

struct Sql_text_format : Text_format
{
                                    Sql_text_format() : Text_format(), _field_quote( '\0' ) {}

    void                            sql_dialect ( const Sos_string& dialect ) { if ( dialect == "odbc" ) field_quote( '\"' ); else field_quote( '\0' ); }

    Text_format&                    field_quote              ( char c )   { _field_quote = c; return *this; }
    char                            field_quote              () const     { return _field_quote; }
 private:
    char                            _field_quote;

};

struct Sql_file : Abs_file
{
                                    Sql_file();
                                    //Sql_file( const char*, Open_mode, const File_spec& );
                                   ~Sql_file();

    void                            open( const char*, Open_mode, const File_spec& );
    void                            close( Close_mode );

//    void                            store( const Const_area& area );
    void                            insert( const Const_area& area );
    void                            update( const Const_area& area );
    void                            store( const Const_area& area );

    void                            set( const Key& );
    void                            del( const Key& key );
    void                            del();

  //Record_position                 key_position (Key::Number = 0) { return _key_position; };
  //Record_length                   key_length   (Key::Number = 0) { return _key_length; };

    static const Abs_file_type&     static_file_type();

    const Sos_string&               tablename();

 protected:
    void                            get_record( Area& area );
    void                            get_record_key( Area& area, const Key& key );

    void                            put_record( const Const_area& area );

 private:
    void                            _init();
    void                            _open_db();
    void                            _replace_select_all( Dynamic_area* );
    void                            _modify_fields( const Sos_string& );

    void                            rollback();
    void                            commit();

    void                            print_value( const Field_descr& f, const Byte* p, ostream* s, Area* hilfspuffer );
    void                            print_field_names( ostream* s, const Field_type& record_type, Bool=false, const char* = "" );
    //void                            print_update_field_names( ostream* s, const Field_type& record_type,
    //                                                          const Const_area& area, const char* name = "", int offs = 0 );
    void                            print_update_field_names( ostream*, const Field_descr&,
                                                              const Const_area& record );
    void                            print_update_field_names( ostream*, const Record_type&,
                                                              const Const_area& record );


    void                            print_where_field_names( ostream* s, const Const_area& area );

    void                            build_update( ostream*, const Const_area& area );
    void                            build_insert( ostream*, const Const_area& area );


    Fill_zero                      _zero_;
    Dynamic_area                   _current_key;

    Sos_string                     _tablename;
    Any_file                       _file;
    Open_mode                      _open_mode;
    Sos_string                     _filename;
    File_spec                      _file_spec;
    Bool                           _opened;
    Bool                           _odbc_mode;

    int                            _required_length;        // minimale Satzlänge bis zum Schlüssel

    //jz30.11.95 const Field_type*              _record_type_ptr;
    //jz30.11.95 const Field_descr*             _key_descr_ptr;
    Sos_ptr<Record_type>           _record_type_ptr;
    Sos_ptr<Record_type>           _write_record_type_ptr;
    Sos_ptr<Field_descr>           _key_descr_ptr;
    Sos_ptr<Record_type>           _key_type;           // wenn _key_descr_ptr->field_count() > 0
    Sql_text_format                _format;

    Bool                           _auto_commit;
    int                            _store;
    Bool                           _commit_at_end;
    Bool                           _store_by_delete;
    Bool                           _single_select;
    Bool                           _null_value_optimizing;
    Bool                           _empty_is_null;
};

#endif
