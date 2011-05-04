// objfile.h
//                                                    (c) SOS GmbH Berlin

#ifndef __OBJFILE_H
#define __OBJFILE_H

#include "../kram/sosobj.h"

namespace sos {


struct Object_file : Abs_file
{
    BASE_CLASS( Abs_file )

    static const Abs_file_type& static_file_type        ();
    virtual Record_position     key_position            ( Key::Number = 0 ) { return _key_pos; }
    virtual Record_length       key_length              ( Key::Number = 0 ) { return _key_len; }

  protected:
    void                        open                    ( const char* filename, Open_mode, const File_spec& );
    void                        close                   ( Close_mode = close_normal );
    virtual void                get_record              ( Area& );
    virtual void                get_record_key          ( Area&, const Key& );
    virtual void                set                     ( const Key& );
    virtual void                put_record              ( const Const_area& );
#if !defined SYSTEM_RTTI
    void                       _obj_print               ( ostream* s ) const         { *s << "Object_file"; }
#endif

  private:
    Sos_object_ptr             _object_ptr;
    Const_area_handle          _current_key; 
};

} //namespace sos
#endif

