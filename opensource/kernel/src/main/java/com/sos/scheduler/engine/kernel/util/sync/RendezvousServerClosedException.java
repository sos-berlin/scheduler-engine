package com.sos.scheduler.engine.kernel.util.sync;


/**
 *
 * @author Zschimmer.sos
 */
public class RendezvousServerClosedException extends RendezvousException {
    RendezvousServerClosedException() {
        super("Rendezvous server has been closed without leaving the rendezvous");
    }
}
