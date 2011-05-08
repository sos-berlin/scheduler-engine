// ddefile.h
// 14. 3.92                                             (c) SOS GmbH Berlin
//                                                      Joacim Zschimmer

#ifndef __DDEFILE_H
#define __DDEFILE_H

#ifndef __ABSFILE_H
#   include "absfile.h"
#endif

namespace sos {

struct Dde_file : Abs_indexed_file, Sos_dde
{
                                Dde_file                ();
                               ~Dde_file                ();

    void                        open                    ( const char* filename, Open_mode, const File_spec& );
    void                        close                   ( Close_mode = close_normal );
    void                        rewind                  ( Key::Number = 0 );

    virtual void                store                   ( const Const_area& );
    virtual void                store_key               ( const Const_area&, const Key& );
          //Record_position     key_position            ( Key::Number = 0 );
          //Record_length       key_length              ( Key::Number = 0 );

    static const Abs_file_type& static_file_type        ();

  protected:
    virtual void                get_record              ( Area& );
    virtual void                get_record_key          ( Area&, const Key& );
    virtual void                put_record              ( const Const_area& );

  private:
    void                       _init                    ();
    void                       _dde_poke_or_execute     ( const Const_area&, HSZ item, uint type );

    HCONV                      _conv_handle;
    int4                       _timeout_ms;
};

} //namespace sos
#endif
