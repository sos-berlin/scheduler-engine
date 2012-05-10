package com.sos.scheduler.engine.cplusplus.runtime;


import static java.lang.String.format;
import static java.util.Locale.ROOT;

public class CppProxyImpl<SISTER extends Sister> implements CppProxyWithSister<SISTER> {
    private static final long nullCppReference = 0;
    /** cppReference wird über JNI gesetzt und gelöscht. Damit hat keine Java-Klasse Zugriff darauf. */
    private volatile long cppReference = nullCppReference;
    private SISTER sister = null;

    protected final long cppReference() {
        long result = cppReference;
        if (result == nullCppReference)
            throw new CppProxyInvalidatedException("Using reference to destroyed C++ object: "+ this);
        return result;
    }

    @Override public boolean cppReferenceIsValid() {
        return cppReference != nullCppReference;
    }

    /** Nur für JNI zugänglich. Wird vom C++-Destruktor ~Has_proxy aufgerufen. */
    @SuppressWarnings("unused")
    private void invalidateCppReference() {
        cppReference = nullCppReference;
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

    @Override public String toString() {
        long ref = cppReference;
        return getClass().getSimpleName() +
                "("+ (ref == nullCppReference? "INVALIDATED" : format(ROOT, "%x", ref)) +")";

    }

    /* Wenn C++ ein temporäres Objekt liefert und also den C++-Proxy über JNI sofort wieder freigibt,
     * dann haben wir nur ein (zerstörtes) Ding der Klasse Object. */
    protected static void checkIsNotReleased(Class<?> clas, Object o) {
        if (o != null && !clas.isInstance(o))
            throw new CppProxyInvalidated(clas);
    }

    private static class CppProxyInvalidated extends RuntimeException {
        private CppProxyInvalidated(Class<?> c) {
            super("C++ code has returned a temporary, immediately destructed object (was a " + c.getName()+")");
        }
    }
}
