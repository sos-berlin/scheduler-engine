/* wprtfile.h                                           (c) SOS GmbH Berlin
                                                            Joacim Zschimmer
*/

#ifndef __WPRTFILE_H
#define __WPRTFILE_H

#if !defined( __SV_HXX )
#   include <sv.hxx>
#endif

#include <absfile.h>




struct Print_file : Abs_record_file
{
                Print_file      () {};
              //Print_file      ( const char* filename, Open_mode ) {};
               ~Print_file      () {};

    static const Abs_file_type& static_file_type();
    static Sos_ptr<Print_file>  create();

    void        open            ( const char* filename, Open_mode, const File_spec& ) {};
    void        close           ( Close_mode = close_normal ) {};
    void        flush           () {};

  protected:
    virtual void put_record     ( const Const_area& ) {};
    virtual void write_area     ( const Const_area& ) {};
    virtual void read_area      ( Area& ) {};

  private:
    void        _init           ();

    Point               _position;
    int                 _left_margin;
    Bool                _opened                : 1;
    Bool                _opened_by_constructor : 1;
};


struct Fontwidth_file : Abs_file
{
                        Fontwidth_file      () { _init(); };
                      //Fontwidth_file      ( const char* filename, Open_mode, const File_spec& );
                       ~Fontwidth_file      () {};

    void                open                ( const char* filename, Open_mode, const File_spec& );

  protected:
    virtual void        get_record_key      ( Area&, const Key& );
  //virtual Record_length       key_length              ( Key::Number = 0 );

  private:
    void                _init();
  //Record_length       _key_length;
};

#endif
