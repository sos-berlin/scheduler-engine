#include "absfile.h"


struct Any_file;
//struct Any_record_file;
//struct Any_indexed_file;

struct __huge Prot {
    void report( const char* function,
		 const char* filename);
    void report( const char* text );
};

/*
struct Prot_file : Prot,
                   Abs_file_base
{
  public:

    virtual void                open                    ( const char*, Open_mode open_mode, const File_spec& );
    virtual void                close                   ( Close_mode  = close_normal );

  private:
    Ptr<Any_file>              _f;
    String0_ptr                _filename;
};

//
struct Prot_record_file : public Prot,
			  public Abs_record_file
{
  // Prot_file - spezifischer Teil
  virtual void close (
      Close_mode close_mode = close_normal
  );

  // Prot_record - spezifischer Teil ...
  virtual void open(
      const char* filename,
      Open_mode         open_mode = in
  );

protected:
  virtual void put_record( Const_area );
  virtual void get_record( Area& );

private:
    Ptr<Any_record_file>   _f;
    String0_ptr                _filename;
};
*/

struct Prot_file : Prot,
                   Abs_indexed_file
{
    // reicht fÅr alle Prot_files (record & indexed)
    static const Abs_file_type& static_file_type();

    // Prot_file - spezifischer Teil
    virtual void                open                    ( const char*, Open_mode, const File_spec& );
    virtual void                close                   ( Close_mode close_mode = close_normal );

  // Prot_record - spezifischer Teil ...
  /*virtual void put (
	//const void     *record,
	//Record_length   record_length
	Const_area area
  );*/


  //jz virtual void                get                     ( Area&, Record_lock );
  /*{
       get( buffer, size, length, 0 );

  }*/


  // Prot_indexed - spezifischer Teil ...

    virtual void                rewind                  ( Key::Number );
    virtual void                put                     ( Const_area area )
    {
        store( area );
    };

  virtual void insert( const Const_area& );
  virtual void store ( const Const_area& );
  virtual void update( const Const_area& );
  virtual void set   ( const Key& key    );
  virtual void del   ();
  virtual void del   ( const Key& key    );
  virtual void lock  ( const Key& key,
		       Record_lock lock = write_lock );
  virtual Record_position key_position( Key::Number = 0);
  virtual Record_length   key_length  ( Key::Number = 0);

protected:
  virtual void get_record( Area& );
  virtual void get_record_key( Area&, const Key& key );
  virtual void get_record_lock( Area&, Record_lock );

private:
    Any_file               _f;
    char*                  _filename;

};


typedef Prot_file Prot_record_file;
typedef Prot_file Prot_indexed_file;

