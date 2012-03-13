/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.test.jobs;

import sos.spooler.Job_impl;

public class JavaTestJob extends Job_impl {
    
    private final static int sleep = 60;

    public boolean spooler_process() throws Exception {
        spooler_log.info("java job started (waiting for " + sleep + " seconds).");
        SimpleThread t1 = new SimpleThread(sleep);
        SimpleThread t2 = new SimpleThread(sleep);
        SimpleThread t3 = new SimpleThread(sleep);
        t1.start();
        t2.start();
        t3.start();
        Thread.sleep(sleep * 1000);
        spooler_log.info("java job ended.");
        return false;
    }

}
