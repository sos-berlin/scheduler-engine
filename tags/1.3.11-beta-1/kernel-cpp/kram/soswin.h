// soswin.h                                             (c) SOS GmbH Berlin
//                                                      Joacim Zschimmer

#if defined SYSTEM_STARVIEW

#ifndef __SOSWIN_H
#define __SOSWIN_H

struct Key_code
{
    enum Key {
        // Code wie ISO. Taste a/A: 'A' (0x41), Taste 1/!: '1' (0x31),
        // Taste +/*: '+', Return: 0x0D
        // Nur die Tasten, für die es keine ISO-Codierung gibt, werden aufgeführt:
        f1             = 0x101,
        f2             = 0x102,
        f3             = 0x103,
        f4             = 0x104,
        f5             = 0x105,
        f6             = 0x106,
        f7             = 0x107,
        f8             = 0x108,
        f9             = 0x109,
        f10            = 0x10A,
        f11            = 0x10B,
        f12            = 0x10C,
        print          = 0x10D,
        scroll         = 0x10E,
        pause          = 0x10F,
        insert         = 0x110,
        del            = 0x111,
        home           = 0x112,
        end            = 0x113,
        page_up        = 0x114,
        page_down      = 0x115,
        left           = 0x116,
        right          = 0x117,
        up             = 0x118,
        down           = 0x119,
        num_lock       = 0x11A,
        num_divide     = 0x11B,
        num_multiply   = 0x11C,
        num_minus      = 0x11D,
        num_plus       = 0x11E,
        num_enter      = 0x11F,
        num_0          = 0x120,
        num_1          = 0x121,
        num_2          = 0x122,
        num_3          = 0x123,
        num_4          = 0x124,
        num_5          = 0x125,
        num_6          = 0x126,
        num_7          = 0x127,
        num_8          = 0x128,
        num_9          = 0x129,
        num_point      = 0x12A,
        _key_mask      = 0x1FF
    };

    enum Modifier {
        none               = 0x0000,
        shift_left         = 0x0200,
        shift_right        = 0x0400,
        shift              = 0x0600,
        alt                = 0x0800,
        alt_gr             = 0x1000,
        cntrl_left         = 0x2000,
        cntrl_right        = 0x4000,
        cntrl              = 0x6000,
        _modifier_mask     = 0x7E00      
    };

    typedef uint2 Value;        // Modifier | Key


                                Key_code        ();
                                Key_code        ( Value, char );
                                Key_code        ( unsigned short, unsigned short, char );  // StarView
    char                        chr             () const; // 0, wenn kein ISO-Zeichen
    Value                       value           () const;    
    Value                       normed_value    () const; // Linkes u. rechtes Shift/Cntrl nicht unterschieden
    Key                         key             () const;
    Modifier                    modifier        () const;
    Bool                        shift_pressed   () const;
    Bool                        alt_pressed     () const;
    Bool                        cntrl_pressed   () const;
    Bool                        printable       () const;

  protected:
    void                        chr             ( char );
    void                        value           ( Value );

  private:
    Value                       _value;
    char                        _char;          // 0, wenn kein ISO-Zeichen
};


struct __huge KeyEvent;  // StarView

struct Key_event
{
                                Key_event       ( const KeyEvent& );   // StarView
    Key_code                    key_code        () const;
  //Key_code::Key               key             () const;
  //Key_code::Modifier          modifier        () const;
    int                         count           () const;

  protected:
    Key_code                    _key_code;
    int                         _count;
};


struct Abs_sos_window
{
    virtual                    ~Abs_sos_window  ();
    static Abs_sos_window*      create          ();
    void                        handle_key_event( const Key_event& );

  protected:
    virtual void                _handle_key_event( const Key_event& )            = 0;
};

struct Sos_window : Abs_sos_window
{
    virtual                    ~Sos_window      ();

  protected:
    virtual void                _handle_key_event( const Key_event& )            = 0;
};

typedef void User_event_function( unsigned long, void* );

struct __huge Abs_sos_application
{
    enum Reschedule_status
    {
        reschedule_normal            = 0,     // Nichts ist passiert
        reschedule_msg_processed,             // Eine Nachricht war da
        reschedule_terminate                  // Applikation beenden
    };

                                Abs_sos_application();
    virtual                    ~Abs_sos_application();

    virtual int/*Sun*/          main            ( int argc, char* argv[] );
    void                        execute         ();             // Ereignisschleife
    void                        add_open_event_count( int );    // Anzahl ausstehender Ereignisse ändern
    int                         open_event_count() const;
    Reschedule_status           reschedule      ();             // Andere mal ranlassen
    int                         rescheduled     () const;       // Anzahl aktiver reschedule()
    Bool                        terminate       () const;       // Applikation beenden (z.B. ALT-F4)?
    void                        busy            ( Bool );       // Computer ist beschäftigt (Maus ist Sanduhr)
    virtual void                user_event_function( User_event_function* ) = 0;

  protected:
    // Von der Applikation zu implementieren:
    virtual void                _main           ( int argc, char* argv[] )      = 0;

    // Von der Schnittstelle zum Fenstersystem zu implementieren:
    virtual void                _execute        ()                              = 0;
    virtual Reschedule_status   _reschedule     ()                              = 0;
    virtual void                _busy           ( Bool )                        = 0;

  private:
    int                         _rescheduled;   // Verschachtelungszähler
    Bool                        _terminate;
    int                         _open_event_count;
};


struct Sos_application : Abs_sos_application
{
    //typedef Abs_sos_application::Reschedule_status Reschedule_status;

                                Sos_application();
    virtual                    ~Sos_application();

    virtual void                user_event_function( User_event_function* );

  protected:
    virtual void                _main           ( int argc, char* argv[] )      = 0;

    virtual void                _execute        ();
    virtual Reschedule_status   _reschedule     ();
    virtual void                _busy           ( Bool );

    Abs_sos_application*        window_system_ptr();

  private:
    Abs_sos_application*        _impl_ptr;
};


extern Sos_application* const  sos_application_ptr;


#include <soswin.inl>
#endif

#endif
