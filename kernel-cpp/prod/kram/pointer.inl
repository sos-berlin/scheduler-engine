// pointer.inl
#if 0

template<class T> Pointer<T>::Pointer( T* ptr )

template<class T> T* Pointer<T>::operator T* () const

template<class T> Pointer  Pointer<T>::operator +  ( int i )
template<T> Pointer  Pointer<T>::operator -  ( int i ) { return _ptr - i; }
template<T> Pointer& Pointer<T>::operator += ( int i ) { return _ptr += i; }
template<T> Pointer& Pointer<T>::operator += ( int i ) { return _ptr -= i; }

#endif
