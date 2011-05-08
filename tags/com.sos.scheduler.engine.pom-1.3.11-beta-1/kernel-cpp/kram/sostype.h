// sostype.h                                               (c) SOS GmbH Berlin

#ifndef __SOSTYPE_H
#define __SOSTYPE_H


struct Member_descr;


struct Sos_type
{
    enum Kind {
        kind_fundamental,       // char, int
        kind_float,             // Nicht fundamental und damit nur bei Bedarf eingebunden
        kind_array,
        kind_string0,
        kind_function,
        kind_reference,
        kind_pointer,
        kind_struct
    };

                                Type                    ( const char* name_ptr, Kind, int size );

    const char*                 name                    () const;
    Kind                        kind                    () const            { return _kind; }
//? void                        repr                    ( const Type_repr* type_repr_ptr ) { _type_repr_ptr = type_repr_ptr; }
//? Type_repr&                  repr                    () const                           { _return *_type_repr_ptr;     }

    // Lesbar als text:
  //virtual void                print_value             ( ostream&, const void* ) const = 0;                // Lesbare Textausgabe
  //virtual void                input_value             ( istream&, void*       ) const = 0;                // Lesbare Texteingabe

    int                         size                    () const        { return _size; }

  private:
    const char* const          _name_ptr;
    const Kind                 _kind;
    const int                  _size;
//? Type_repr*                 _type_repr_ptr;       // Zeiger auf die Darstellungsbeschreibung
};


struct Fundamental_type : Sos_type
{
    enum Plain_type
    {
        plain_char,
        plain_int
    //, plain_float
    };

                                Fundamental_type        ( Plain_type, int size, Bool signed = true );

  //int                         size                    () const;
    Bool                        signed                  () const;

  //void                        print_value             ( ostream&, const void*  ) const;    // Lesbare textausgabe
  //void                        input_value             ( istream&, void*        ) const;    // Lesbare texteingabe

  private:
    const Plain_type           _plain_type;
  //const int                  _size;
    const Bool                 _signed;
};


struct Array_type : Sos_type
{
                                Type_array              ( const Type*, int no_of_elements, int element_distance = 0 );

  //void                        print_value             ( ostream&, const void*  ) const;    // Lesbare textausgabe
  //void                        input_value             ( istream&, void*        ) const;    // Lesbare texteingabe
  //int                         size                    () const;
    int                         no_of_elements          () const;
    int                         element_distance        () const;
    const Type&                 element_type            () const;

  private:
    const Type*                _type_ptr;
    const int                  _no_of_elements;
    const int                  _element_distance;
};

/*
struct Type_function : Type
{
                                Type_function           ( const Type* return_type_ptr, Type* parameters[], int no_of_parameters );

  //void                        print_value             ( ostream&, const void*  ) const;    // Lesbare textausgabe
  //void                        input_value             ( istream&, void*        ) const;    // Lesbare texteingabe
  //int                         size                    () const;
};


struct Type_reference : Type
{
                                Type_reference          ( const Type* );
    int                         size                    () const;

  private:
    const Type*                _type_ptr;
};


struct Type_pointer : Type
{
                                Type_pointer            ( const Type* );

    int                         size                    () const;
    void                        print_value             ( ostream&, const void*  ) const;    // Lesbare textausgabe
#ifndef NO_INPUT
    void                        input_value             ( istream&, void* ) const;    // Lesbare texteingabe
#endif

  protected:
    const Type*                _type_ptr;
};


struct Type_const : Type
{
                                Type_const              ( const Type* );

    int                         size                    () const;
    void                        print_value             ( ostream&, const void*  ) const;    // Lesbare textausgabe
#ifndef NO_INPUT
    void                        input_value             ( istream&, void* ) const;    // Lesbare texteingabe
#endif
};
*/

struct Struct_type : Type
{
/*
    enum What
    {
        data_only,
        readable,
        readable_with_names
    };
*/

                                Struct_type             ( const char* name_ptr, int size, const Member_descr[], int no_of_members );

    const Member_descr&         member_descr            ( int ) const;
    int                         no_of_members           () const;
    void                        add_member              ( const Struct_member_descr& );

  //int                         size                    () const;
  //void                        print_value             ( ostream&, const void*  ) const;    // Lesbare textausgabe
  //void                        input_value             ( istream&, void*        ) const;    // Lesbare texteingabe

  private:
    const int                  _size;
    Sos_simple_array<Member_descr> _member_descr_array;
};


struct Field_descr
{
                                Field_descr             ( const char* name, const Type* );

    const char*                 name                    () const;
    const Type&                 type                    () const;

  //void                        print_value             ( ostream&, const void* ) const;    // Lesbare textausgabe
  //void                        input_value             ( istream&, void* ) const;    // Lesbare texteingabe

  private:
    const char*                _name;
    const Type*                _type_ptr;
};


struct Struct_member_descr : Field_descr
{
                                Struct_member_descr     ( const Type*, int offset, const char* name, const char* ext_name = "" );

    int                         offset                  () const;

    //? void                    print_value             ( ostream&, const void* struct_ptr ) const;

  private:
    const int                  _offset;
};

/* Ausgaben: Externe Darstellung: wie der Objektinhalt über seine Methoden sichtbar ist.
             Interne Darstellung: Die (privaten) Felder zur Diagnose
         .............
   Darstellung als Text, binär (für Persistenz) und als Objekt-Window.
*/

/*
struct Type_owner_pointer : Type_pointer
{
                                Type_owner_pointer      ( const Type* );

    int                         size                    () const;
  //void                        print_value             ( ostream&, const void*  ) const;    // Lesbare textausgabe
  //void                        input_value             ( istream&, void* ) const;    // Lesbare texteingabe
};
*/
/*
struct Type_string0 : Type
{
                                Type_string0            ( int size );

    void                        print_value             ( ostream&, const void* ) const;    // Lesbare textausgabe
#ifndef NO_INPUT
    void                        input_value             ( istream&, void* ) const;    // Lesbare texteingabe
#endif
    int                         size                    () const;

  private:
    const int                  _size;
};
*/
//-------------------------------------------------------------------------------------------------
#if 0

struct Typed_not_virtual_without_destructor
{
  protected:
    Typed_not_virtual_without_destructor( const Type* );
};


struct Typed_not_virtual : Typed_not_virtual_without_destructor
{
    // Dieser Typ belegt ein Byte, vielleicht für den Konstruktor.
    static void                 dump_all_objects        ( ostream& );

  protected:
                                Typed_not_virtual       ( const Type* );
                               ~Typed_not_virtual       ();
};

struct __huge Object_window;

struct Typed : Typed_not_virtual
{
    virtual const Type_struct&  object_type             () const = 0;
    virtual void                object_print            ( ostream& ) const;
#ifndef NO_INPUT
    virtual void                object_input            ( istream& ) const;
#endif

#if defined SYSTEM_STARVIEW
    static  Object_window*      object_show             ( const void* object_ptr, const Type* );
    virtual Object_window*      object_show             () const;           // In einem eigenen Fenster zeigen
#endif

    static void                 insert_object           ( const void* object_ptr, const Type* );
    static void                 remove_object           ( const void* object_ptr );
    static void                 dump_all_objects        ( ostream& );

  protected:
#if 1
                                Typed                   ( const Type* t ) : Typed_not_virtual( /*&type()*/ t )  {}
#else
                                Typed                   ();
                               ~Typed                   ();

  private:
    Typed*                     _next_typed_object;
    static Typed*              _object_list;
#endif
};

#if defined( SYSTEM_STARVIEW )

struct __huge Window;  // StarView

struct __huge Object_window
{
    virtual                     ~Object_window          () = 0;
    static Object_window*        create                 ( Window*, void* object_ptr, Type* );
    virtual void                 refresh                () = 0;
};

#endif


inline ostream& operator<< ( ostream& s, const Typed& object )
{
    object.object_print( s );
    return s;
}

#ifndef NO_INPUT
inline istream& operator>> ( istream& s, Typed& object )
{
    object.input( s );
    return s;
}
#endif

#endif
//------------------------------------------------------------------------------------------#define

#define DECLARE_STRUCT_INFORMATION( no_of_members ) \
  public:                                         \
    const Type_struct& object_type() const;       \
    static const Type_struct _static_type;        \
  private:                                        \
    static const Member_descr _member_descr[ no_of_members ];

#define DEFINE_STRUCT_INFORMATION( struct_name ) \
    const Type_struct struct_name::_static_type ( #struct_name,                            \
                                                  sizeof (struct_name),                    \
                                                  _member_descr, NO_OF( _member_descr ) ); \
    const Type_struct& struct_name::object_type() const { return _static_type; };          \
    const Member_descr struct_name::_member_descr [] =

#define DEFINE_STRUCT_MEMBER( struct_name, name, type_ptr )  \
    Member_descr( #name, type_ptr, (uint) ((char*)&((struct_name*)0)->name - (char*)0))


//------------------------------------------------------------------------const

extern const Type_fundamental type_char;
extern const Type_fundamental type_int;
extern const Type_fundamental type_int1;
extern const Type_fundamental type_int2;
extern const Type_fundamental type_int4;
extern const Type_fundamental type_long;
extern const Type_fundamental type_uint;
extern const Type_fundamental type_uint1;
extern const Type_fundamental type_uint2;
extern const Type_fundamental type_uint4;
extern const Type_fundamental type_ulong;
//extern const Type_fundamental type_float;


#include "type.inl"

