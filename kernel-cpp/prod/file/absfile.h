// absfile.h                                             (c) 1992-1995 SOS GmbH Berlin
//
// Abstrakte Klassen für in Sätzen organisierte Dateien

#ifndef __ABSFILE_H
#define __ABSFILE_H

#if !defined __SOSOBJ_H
#   include "../kram/sosobj.h"
#endif

#if !defined __FILEBASE_H
#   include "../file/filebase.h"
#endif

namespace sos
{

//-----------------------------------------------------------------------------------------Abs_file

struct SOS_CLASS Any_file_obj;

struct Abs_file : Sos_object,  File_base
{
    BASE_CLASS( Sos_object )
    DEFINE_OBJ_IS_TYPE( Abs_file )

                                Abs_file                ();
    virtual                    ~Abs_file                ();

    virtual void                prepare_open            ( const char* filename, Open_mode, const File_spec& );
    virtual void                bind_parameters         ( const Record_type*, const Byte* );
    virtual void                open                    ( const char* filename, Open_mode, const File_spec& );
    virtual void                close                   ( Close_mode = close_normal );
    virtual void                execute                 ();

//            void                put                     ( const Const_area& area )                        { put_record( area ); }
//            void                put                     ( unsigned char );

//            void                get                     ( Area* area_ptr )                                { get_record( *area_ptr ); }
//            void                get                     ( Area* area_ptr, Record_lock record_lock )       { get_record_lock( *area_ptr, record_lock ); }
//            void                get                     ( Area* area_ptr, const Key& key          )       { get_record_key( *area_ptr, key );         }
//            void                get_key                 ( Area* area_ptr, const Key& key          )       { get_record_key( *area_ptr, key );         }
///*obsolete*/void                get                     ( Area& area )                                    { get_record( area ); }
///*obsolete*/void                get                     ( Area& area, Record_lock record_lock )           { get_record_lock( area, record_lock ); }
///*obsolete*/void                get                     ( Area& area, const Key& key          )           { get_record_key( area, key );         }
//    virtual void                get_first               ( Area&, Key::Number = 0 );

    virtual void                insert                  ( const Const_area& );
    virtual void                insert_key              ( const Const_area&, const Key& );
    virtual void                store                   ( const Const_area& );
    virtual void                store_key               ( const Const_area&, const Key& );
    virtual void                update                  ( const Const_area& );
    virtual void                update_direct           ( const Const_area& );
    virtual void                set                     ( const Key& );
    virtual void                del                     ();
    virtual void                del                     ( const Key& );
  //virtual void                execute                 ( const Const_area&, Area* );

    virtual void                lock                    ( const Key&, Record_lock = write_lock )  {}
            void                unlock                  ( const Key& key )                               {  lock( key, no_lock ); };

    virtual void                rewind                  ( Key::Number = 0 );

    virtual const Const_area&   current_key             ();

  //virtual Record_position     key_position            ( Key::Number = 0 );
  //virtual Record_length       key_length              ( Key::Number = 0 );
    Record_position             key_position            ( Key::Number = 0 )                             { return _key_pos; }
    Record_length               key_length              ( Key::Number = 0 )                             { return _key_len; }

    virtual void                get_filename            ( Area& area );
    virtual File_info           info                    ();

    virtual void                current_key_ptr         ( const Const_area* );
            const Const_area*   current_key_ptr         () const { return _current_key_ptr; };

    virtual Abs_file*           new_file                ();                       // open() kann die Datei austauschen
    virtual void                any_file_ptr            ( Any_file_obj* );

//protected:

    virtual void                get_record              ( Area& area ); /// { get_record_lock( area, no_lock ); }
    virtual void                get_record_lock         ( Area&, Record_lock );
    virtual void                get_until               ( Area*, const Const_area& );
    virtual void                get_record_key          ( Area&, const Key& );
    virtual void                get_position            ( Area*, const Const_area* until_key = NULL );
    virtual void                put_record              ( const Const_area& area ); ///  { store ( area ); }
  //virtual void                reopen                  ();

  private:
    friend struct               Any_file_obj;

    Fill_zero                  _zero_; 
    const Const_area*          _current_key_ptr;

  protected:
    void                       _obj_print               ( ::std::ostream* ) const;

    Any_file_obj*              _any_file_ptr;
    int                        _key_pos;
    int                        _key_len;
  //vector<Secondary_index>    _secondary_indices;
}; // struct Abs_file

typedef Abs_file Abs_file_base;
typedef Abs_file Abs_record_file;
typedef Abs_file Abs_indexed_file;

typedef Abs_file Abs_connection;     // ???????

//--------------------------------------------------------------------------------Abs_file_type

struct Abs_file_type
/* Pro Dateityp ein Exemplar für alle (Windows-)Tasks
*/
{
                                Abs_file_type           ()              { _next = _head; _head = this; }

    virtual const char*         name                    () const = 0;
    virtual const char*         alias_name              () const        { return ""; };

  //virtual File_type_common*   create_common           () const        { return 0; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const = 0;

    virtual void                erase                   ( const char* file_name );
    virtual void                rename                  ( const char* old_name, const char* new_name );
    virtual void                dummy                   ();            // Was'n das? jz 17.11.93

    static Abs_file_type*      _lookup                  ( const char* name );

  private:
    Abs_file_type*             _next;

    static Abs_file_type*      _head;
};



//#include <absfile.inl>

inline void Abs_file::current_key_ptr( const Const_area* key_ptr )
{
    _current_key_ptr = key_ptr;
}


} //namespace sos

#if !defined __ANYFILE2_H
#   include "../file/anyfile.h"
#   include "../file/anyfile2.h"
#endif


#endif

