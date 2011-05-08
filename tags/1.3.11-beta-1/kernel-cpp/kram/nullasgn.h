// nullasgn.h
// 1.9.92                                                       Joacim Zschimmer

#if defined SYSTEM_DOS

struct Null_pointer_assignment
{
    Null_pointer_assignment();
    void init();
    int operator() ();

  private:
	 Bool initialized;
#  if defined( SYSTEM_DOS )
	 uint4 null_data [256];         // Daten an Adresse 0
#  endif
};

extern Null_pointer_assignment null_pointer_assignment;

#else

inline void null_pointer_assignment() {}

#endif
