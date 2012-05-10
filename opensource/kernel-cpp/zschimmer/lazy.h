// $Id$

#ifndef __ZSCHIMMER__LAZY_H
#define __ZSCHIMMER__LAZY_H

namespace zschimmer {

//----------------------------------------------------------------------------------abstract_lazy<>

template<class T> 
struct abstract_lazy {
    abstract_lazy() : _value(T()) {}

    T& get() const { 
        if (!_value)  
            initialize();  
        return _value; 
    }

    T& operator->() const { return get(); }
    T& operator*() const { return get(); }

  protected:
    virtual void initialize() const = 0;

    mutable T _value;
};

//-------------------------------------------------------------------------------------------lazy<>

template<class T> 
struct lazy : abstract_lazy<T> {
  protected:
    void initialize() const;    // Für jedes T eine Spezialisierung implementieren!
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
