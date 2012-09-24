package com.sos.scheduler.engine.common.sync;


/**
 *
 * @author Zschimmer.sos
 */
public class RendezvousServerClosedException extends RendezvousException {
    RendezvousServerClosedException() {
        super("Rendezvous server has been closed without leaving the rendezvous");
    }
}
