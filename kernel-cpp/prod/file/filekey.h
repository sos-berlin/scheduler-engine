// filekey.h
//                                                    (c) SOS GmbH Berlin

#ifndef __FILEKEY_H
#define __FILEKEY_H

namespace sos {


struct File_as_key_file : Abs_file
{
    BASE_CLASS( Abs_file )

    static const Abs_file_type& static_file_type        ();
  //virtual Record_position     key_position            ( Key::Number = 0 ) { return _key_position; }
  //virtual Record_length       key_length              ( Key::Number = 0 ) { return _key_length; }

  protected:
    virtual void                get_record_key          ( Area&, const Key& );
  //virtual void                put_record              ( const Const_area& );

    Bool                       _block_text; 
};

} //namespace sos

#endif
