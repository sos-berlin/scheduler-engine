package com.sos.scheduler.engine.cplusplus.runtime;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.annotation.Nullable;
import java.util.HashSet;
import java.util.NoSuchElementException;
import java.util.Set;

import static com.google.common.base.Preconditions.checkArgument;
import static com.google.common.base.Preconditions.checkState;
import static java.util.Collections.synchronizedSet;

/** {@link ReleasableCppProxy}, die über die Lebensdauer des C++-Schedulers vom Java-Code gehalten werden,
 * sind hier zu registrieren, damit der Scheduler sie bei seinem Ende freigeben kann.
 * Das Verhindert einen Ansturz im C++-Code wegen verspäterer Freigabe (wegen Zugriffs auf zerstörte C++-Objekten). */
public class DisposableCppProxyRegister {
    private static final Logger logger = LoggerFactory.getLogger(DisposableCppProxyRegister.class);

    /** {@link ReleasableCppProxy}, die freigegeben werden müssen. */
    private final Set<CppReference<?>> register = synchronizedSet(new HashSet<CppReference<?>>());
    private boolean closed = false;

    /** Das C++-Objekt MUSS mit {@link #dispose(CppReference)} wieder freigegeben werden, sonst Speicherleck. */
    public <T extends ReleasableCppProxy> CppReference<T> reference(T o) {
        checkState(!closed, DisposableCppProxyRegister.class.getSimpleName() +".reference("+ o +"): register has been closed");
        CppReference<T> result = new CppReference<T>(o);
        register.add(result);
        return result;
    }

    /** Alle übrigen C++-Objekte freigeben, nur beim Beenden des Schedulers aufzurufen.
     * Das kann zum Absturz führen, ist also nur für bestimmte C++-Objekte verwendbar. */
    public void tryDisposeAll() {
        closed = true;
        while(true) {
            CppReference<?> ref = tryStealAReference();
            if (ref == null)
                break;
            logger.debug("tryDisposeAll(): "+ref);
            try {
                ref.dispose();
            } catch (Exception x) {
                logger.error("tryDisposeAll(): "+x.toString(), x);
            }
        }
    }

    /** Stiehlt thread-sicher eine Referenz. */
    @Nullable private CppReference<?> tryStealAReference() {
        try {
            while(true) {
                CppReference<?> result = register.iterator().next();
                boolean exists = register.remove(result);
                if (exists) return result;
            }
        } catch (NoSuchElementException x) {
            return null;
        }
    }

    public <T extends ReleasableCppProxy> void dispose(CppReference<T> o) {
        if (closed)
            logger.debug("Ignored after disposeAll(): dispose({})", o);
        else {
            boolean existed = register.remove(o);
            checkArgument(existed, DisposableCppProxyRegister.class.getSimpleName() +".dispose(): Unknown reference "+o);
            o.dispose();
        }
    }
}
