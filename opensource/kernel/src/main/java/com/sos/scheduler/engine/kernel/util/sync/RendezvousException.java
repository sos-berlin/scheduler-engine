package com.sos.scheduler.engine.kernel.util.sync;

/**
 *
 * @author Zschimmer.sos
 */
public class RendezvousException extends RuntimeException {
    public RendezvousException(String m) { super(m); }
    public RendezvousException(String m, Throwable t) { super(m, t); }
}
