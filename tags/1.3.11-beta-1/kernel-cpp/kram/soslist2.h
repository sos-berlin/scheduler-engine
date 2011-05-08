/* soslist.h                                            (c) SOS GmbH Berlin
                                                        Joacim Zschimmer
*/

#ifndef __SOSLIST_H
#define __SOSLIST_H

/* Sos_simple_list<T> ist eine einfach Listenimplementierung, wie sie aus funktionalen Sprachen
   bekannt ist. Es gibt kein Objekt, das die Liste als Ganzes beschreibt.
   list :  (objekt,list_ptr)
   list_ptr :  null | list*
*/

struct Sos_simple_list_0_elem;

struct Sos_simple_list_0        // Zeiger auf eine Liste oder auf NULL
{
                                Sos_simple_list_0       ();
                                Sos_simple_list_0       ( Sos_simple_list_0_elem* elem_ptr );
    virtual                    ~Sos_simple_list_0       ();

    Bool                        empty                   () const;
    int                         length                  () const;
    Sos_simple_list_0_elem*     operator->              () const;
    Sos_simple_list_0_elem*     elem_ptr                () const;
    void                        elem_ptr                ( Sos_simple_list_0_elem* );

  protected:
  //                            Sos_simple_list_0       ( const Sos_simple_list_0& );

  //Sos_simple_list_0           tail                    () const;
  //Sos_simple_list_0           last                    () const;

  private:
    Sos_simple_list_0_elem*    _elem_ptr;
};

/*
struct Sos_referenced_counted
{
                                Sos_referenced_counted  () : _ref_count( 1 )  {}
};
*/

struct Sos_simple_list_0_elem        // Liste ohne Objekte, Helferklasse für Sos_simple_list<T>
{
    virtual                    ~Sos_simple_list_0_elem  ();

  protected:
                                Sos_simple_list_0_elem  ();
                                Sos_simple_list_0_elem  ( const Sos_simple_list_0& tail );
    //                          Sos_simple_list_0_elem  ( const Sos_simple_list_0_elem& );  // Deep copy!
    Sos_simple_list_0           tail                    () const;
    Sos_simple_list_0           last                    ();
    Sos_simple_list_0_elem*     last_elem_ptr           ();

    Sos_simple_list_0          _tail;
    int                        _ref_count;

  friend class Sos_simple_list_0;
};


template< class T >  struct Sos_simple_list_elem;

template< class T >
struct Sos_simple_list : Sos_simple_list_0
{
                                Sos_simple_list         ();                     // Empty list
  //                            Sos_simple_list         ( const Sos_simple_list_0& );
                                Sos_simple_list         ( const T&, const Sos_simple_list<T>& tail );
                                Sos_simple_list         ( const T& );
  //?                           Sos_simple_list         ( Sos_simple_list_elem<T>* );

    Bool                        empty                   () const;
    int                         length                  () const;
    Sos_simple_list_elem<T>*    operator->              () const;
  //T                           head                    () const;
  //T*                          head_ptr                () const;
  //Sos_simple_list<T>          tail                    () const;
  //Sos_simple_list<T>          last                    ();
  //T*                          element_ptr             ( int );
    void                        insert                  ( const T& );
    void                        append                  ( const T& );

    void                        store                   ( ostream& );
    Sos_simple_list_elem<T>&    load                    ( istream& );

  protected:
    Sos_simple_list_0_elem*     elem_ptr                () const;
};


template< class T >
struct Sos_simple_list_elem : Sos_simple_list_0_elem
{
                                Sos_simple_list_elem    ( const T&, const Sos_simple_list<T>& tail );
                                Sos_simple_list_elem    ( const T& );

    T                           head                    () const;
    T*                          head_ptr                () const;
    Sos_simple_list<T>          tail                    () const;
    Sos_simple_list<T>          last                    () const;
  //T*                          element_ptr             ( int );
    void                        append                  ( const T& );
                                operator Sos_simple_list<T> () const;

  private:
    Sos_simple_list_0          _tail;
    T                          _object;

  friend class Sos_simple_list<T>;
  friend class Sos_simple_list_0;
  friend class Sos_simple_list_0_elem;
};

#include "soslist.inl"
#endif