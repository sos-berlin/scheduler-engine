package com.sos.scheduler.engine.kernelcpptest.jira.js674;

import org.apache.log4j.Logger;
import org.junit.Test;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.util.OperatingSystem;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;


//FIXME Test wiederbeleben!
/** Stellt sicher, dass ein Shell-Prozess einen Kind-Prozess mit kill abbrechen kann.
 *
 * @author Joacim Zschimmer
 */
public class Js674Test extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(Js674Test.class);
    private static final Time myTimeout = Time.of(10+5);   // Länger als der Job im Fehlerfall läuft. Siehe blockedSignal.job.xml


//    @Test public void test() throws Exception {
//        if (OperatingSystem.isUnix) {   // Sollte der Windows-Scheduler die Cygwin-bash starten, wenn das Skript mit "#! /bin/bash" beginnt?
//            MyEventSubscriber eventSubscriber = new MyEventSubscriber();
//            controller().subscribeEvents(eventSubscriber);
//            controller().runScheduler(myTimeout);
////            assertThat(eventSubscriber.resultOk, equalTo(true));
//        }
//    }
//
//
//    private class MyEventSubscriber implements EventSubscriber {
//        private volatile boolean resultOk = false;
//
//        @Override public void onEvent(Event event) {
////            if (event instanceof TaskTerminatedEvent) {
////                TaskTerminatedEvent e = (TaskTerminatedEvent)event;
////                resultOk = e.getExitCode() == 0;
////                schedulerController.terminateScheduler();
////            }
//        }
//    }
}
