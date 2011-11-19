// typereg.h                        ©1995 SOS GmbH Berlin           Joacim Zschimmer

#ifndef __TYPEREG_H
#define __TYPEREG_H

namespace sos
{

struct Type_register : Sos_self_deleting
{
    struct Entry
    {
                                Entry                   ();
                               ~Entry                   ();

        Sos_string             _name;
        Sos_ptr<Field_type>    _type;
    };

                                Type_register           ();
                               ~Type_register           ();

    void                        add                     ( const Sos_ptr<Field_type>&, const char* name );
    void                        add                     ( const Sos_ptr<Field_type>&, const Sos_string& name );
    void                        add                     ( const Sos_ptr<Record_type>& );

    Sos_ptr<Field_type>         type                    ( const char* );
    Sos_ptr<Field_type>        _type                    ( const char* );   // ohne Exception
    Sos_ptr<Record_type>        record_type             ( const char* );

  private:
    Fill_zero                  _zero_;
    int                        _last_used_index;
    Sos_simple_array<Entry>    _array;
};


Type_register* type_register_ptr();

} //namespace sos

#endif

