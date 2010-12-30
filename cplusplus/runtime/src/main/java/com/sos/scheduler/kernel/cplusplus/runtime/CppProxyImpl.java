package com.sos.scheduler.kernel.cplusplus.runtime;


public class CppProxyImpl<SISTER extends Sister> implements CppProxyWithSister<SISTER> {
    /** cppReference wird über JNI gesetzt und gelöscht. Damit hat keine Java-Klasse Zugriff darauf. */
    private volatile long cppReference = 0;
    private SISTER sister = null;


    protected final long cppReference() { return cppReference; }
    @Override public boolean cppReferenceIsValid() { return cppReference != 0; }

    
    /** Nur für JNI zugänglich. Wird vom C++-Destruktor ~Has_proxy aufgerufen. */
    @SuppressWarnings("unused")
    private void invalidateCppReference() {
        cppReference = 0;

        if (sister != null)
            sister.onCppProxyInvalidated();
    }


    @Override public final void setSister(SISTER o) { sister = o; }

    @Override public final SISTER getSister() {
        if (sister == null)  throw new NullPointerException("Has no sister object: " + this);
        return sister;
    }

    @Override public final boolean hasSister() { return sister != null; }


    protected void requireContext(Sister context) {
        if (context == null)  throw new RuntimeException("Missing Sister context in " + getClass().getName());
    }


    protected void requireContextIsNull(Sister context) {
        if (context != null)  throw new RuntimeException("Sister context != null, but interface has no sisterType, in " + getClass().getName());
    }
}
