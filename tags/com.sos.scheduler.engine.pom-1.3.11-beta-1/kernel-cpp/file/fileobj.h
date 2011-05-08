// fileobj.h                    © 1995 SOS GmbH Berlin

#ifndef __FILEOBJ_H
#define __FILEOBJ_H

#if !defined __SOSFILTR_H
#   include "../kram/sosfiltr.h"
#endif

#if !defined __RAPID_H
#   include "../fs/rapid.h"
#endif

#if !defined __ABSFILE_H
#   include "absfile.h"
#endif

namespace sos {

//-------------------------------------------------------------------------------------Sos_file

struct Sos_file : Sos_msg_filter // ?
{
    BASE_CLASS( Sos_msg_filter )

  protected:
    Bool                       _obj_is_type             ( Sos_type_code t ) const   { return t == tc_Sos_file || Base_class::_obj_is_type( t ); }
};

//-------------------------------------------------------------------------------Get_direct_msg

struct Get_direct_msg : Get_msg
{
                                Get_direct_msg          ( Sos_object*, Sos_object*,
                                                          const Const_area_handle& key );

    const Const_area_handle&    key                     () const  { return _key; }  
    Rapid::Parflget::Flags     _flags;
    uint4                      _position;       // -1, oder seek()-Wert oder Satznummer
    Const_area_handle          _key;
  //Const_area_handle          _key_end;
  //Sos_string                 _name;           // evtl. SQL-SELECT?
    int4                       _record_count;   // Geschätzte Anzahl erwarteter Sätze
};

typedef Get_direct_msg T_get_direct_msg;

//------------------------------------------------------------------------------------Store_msg
/*
struct Store_msg : Data_msg
{
    Rapid::Parflput::Flags     _flags;
    uint4                      _position;       // -1, oder seek()-Wert oder Satznummer
    Const_area_handle          _key;
};
*/
////////////////////////////////////////////////////////////////////////////////////////INLINES

inline Get_direct_msg::Get_direct_msg( Sos_object* dest, Sos_object* source,
                                       const Const_area_handle& key )
:
    Get_msg ( msg_get_direct, dest, source ),
    _key ( key )
{
};

} //namespace sos
#endif
