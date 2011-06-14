package com.sos.scheduler.engine.cplusplus.runtime;


public class CppProxyImpl<SISTER extends Sister> implements CppProxyWithSister<SISTER> {
    /** cppReference wird über JNI gesetzt und gelöscht. Damit hat keine Java-Klasse Zugriff darauf. */
    private volatile long cppReference = 0;
    private SISTER sister = null;


    protected final long cppReference() {
        return cppReference;
    }

    
    @Override public boolean cppReferenceIsValid() { 
        return cppReference != 0;
    }

    
    /** Nur für JNI zugänglich. Wird vom C++-Destruktor ~Has_proxy aufgerufen. */
    @SuppressWarnings("unused")
    private void invalidateCppReference() {
        cppReference = 0;
        if (sister != null)
            sister.onCppProxyInvalidated();
    }


    @Override public final void setSister(SISTER o) {
        sister = o;
    }


    @Override public final SISTER getSister() {
        if (sister == null)
            throw new NullPointerException("Has no sister object: " + this);
        return sister;
    }

    @Override public final boolean hasSister() { 
        return sister != null;
    }


//    Variable_setC zum Beispiel hat eine kontextfreie Schwester.
//    protected void requireContext(Sister context) {
//        if (context == null)  throw new RuntimeException("Missing Sister context in " + getClass().getName());
//    }


    protected final void requireContextIsNull(Sister context) {
        if (context != null)
            throw new RuntimeException("Sister context != null, but interface has no sisterType, in " + getClass().getName());
    }


    /* Wenn C++ ein temporäres Objekt liefert und also den C++-Proxy über JNI sofort wieder freigibt,
     * dann haben wir nur ein (zerstörtes) Ding der Klasse Object. */
    protected static final void checkIsNotReleased(Class<?> clas, Object o) {
        if (!clas.isInstance(o))
            throw new CppProxyInvalidated(clas);
    }

    
    private static class CppProxyInvalidated extends RuntimeException {
        private CppProxyInvalidated(Class<?> c) {
            super("C++ code has return a temporary, immediately destructed object (was a " + c.getName());
        }
    }
}
