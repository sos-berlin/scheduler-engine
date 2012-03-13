/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.test.jobs;

import sos.spooler.Log;

/**
 * Created by IntelliJ IDEA.
 * User: schaedi
 * Date: 08.03.12
 * Time: 09:55
 * To change this template use File | Settings | File Templates.
 */
public class SimpleThread extends Thread {
    
    private final int runtimeInSeconds;

    public SimpleThread(int runtimeInSeconds) {
        this.runtimeInSeconds = runtimeInSeconds;
    }

    public void run() {
        System.out.println("thread started: " + this.getId());
        System.out.println("waiting for " + runtimeInSeconds + " seconds.");
        try {
            Thread.sleep(runtimeInSeconds * 1000);
        } catch (InterruptedException e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }
        System.out.println("thread ended: " + this.getId());
    }
}
