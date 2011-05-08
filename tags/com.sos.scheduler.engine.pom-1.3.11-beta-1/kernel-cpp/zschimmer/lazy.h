// $Id$

#ifndef __ZSCHIMMER__LAZY_H
#define __ZSCHIMMER__LAZY_H

namespace zschimmer {

//----------------------------------------------------------------------------------abstract_lazy<>

template<class T> 
struct abstract_lazy {
    abstract_lazy() : _value(T()) {}

    T& get() { 
        if (!_value)  
            initialize();  
        return _value; 
    }

    T& operator->() { return get(); }
    T& operator*() { return get(); }

  protected:
    virtual void initialize() = 0;

    T _value;
};

//-------------------------------------------------------------------------------------------lazy<>

template<class T> 
struct lazy : abstract_lazy<T> {
  protected:
    void initialize();    // Für jedes T eine Spezialisierung implementieren!
};

//-------------------------------------------------------------------------------------------------

}

// Für jedes Spezialisierung des Templates lazy<> muss die Methode initialize() implementiert werden 
// (dabei T durch den konkrete Klassennamen ersetzen):
// namespace zschimmer {
//      void lazy<T>::initialize() {
//          _value = new_T ... ;
//      }
// }

#endif
