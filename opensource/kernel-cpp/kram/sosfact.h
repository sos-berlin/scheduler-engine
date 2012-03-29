// sosfact.h                                            © 1995 SOS GmbH Berlin

#ifndef __SOSFACT_H
#define __SOSFACT_H

#include "soslist.h"
#include "sosobj.h"

namespace sos
{

struct File_spec;

//-----------------------------------------------------------------------------Sos_object_descr

struct Sos_object_descr
{
    typedef int                 Subtype_code;

                                Sos_object_descr        ();

    virtual const char*         name                    () const = 0;
    virtual Subtype_code        is_my_name              ( const char* n ) const { return strcmp( n, name() ) == 0? 1 : 0; }
    virtual Bool                handles_complete_name   () const                { return false; }
    virtual Sos_object_ptr      create                  ( Subtype_code ) const = 0;
};

//------------------------------------------------------------------------Sos_object_descr_node

typedef Sos_simple_list_node<const Sos_object_descr*>  Sos_object_descr_node;

//---------------------------------------------------------------------------------------static

struct Sos_factory : Sos_object
{
    BASE_CLASS( Sos_object )

                                Sos_factory         ()  { _obj_request_semaphore = 999; }
                               ~Sos_factory         ();  //jz 4.2.01

    void                        add                 ( const Sos_object_descr* );
    Sos_object_ptr              request_create      ( Sos_object* sender, const Sos_string& name, Sos_object* owner = 0 );
    Sos_object_ptr              create              ( const Sos_string& name, Sos_object* owner = 0 );

  protected:
  //void                       _obj_create_msg      ( Create_msg* );
#if !defined SYSTEM_RTTI
    void                       _obj_print           ( std::ostream* s ) const  { *s << "Sos_factory"; }
#endif

  private:
    friend struct               Sos_factory_agent;

    Sos_object_descr_node*     _object_descr_head;
};

//---------------------------------------------------------------------------------------extern

extern Sos_factory* sos_factory_ptr();       


} //namespace sos

#endif
