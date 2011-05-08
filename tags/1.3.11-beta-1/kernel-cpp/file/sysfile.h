#include <absfile.h>

struct System_file : public Abs_record_file {

  System_file() {};
  System_file( const char* cmd );

  static const Abs_file_type& static_file_type();

  virtual void open(
      const char*       cmd,
      Open_mode         open_mode = in
  );

  virtual void close (
      Close_mode close_mode = close_normal
  );

protected:
  virtual void put_record ( Const_area area );

  virtual void get_record ( Area& area );

private:
  void dos_command( Const_string0_ptr cmd );
  Bool check_dos_command( Const_string0_ptr cmd );

};

