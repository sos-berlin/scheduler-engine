package com.sos.scheduler.engine.tests.jira.js674;

import com.sos.scheduler.engine.test.SchedulerTest;
import org.joda.time.Duration;

//TODO Test wiederbeleben!
/** Stellt sicher, dass ein Shell-Prozess einen Kind-Prozess mit kill abbrechen kann.
 *
 * @author Joacim Zschimmer
 */
public class Js674IT extends SchedulerTest {
    private static final Duration myTimeout = Duration.standardSeconds(10 + 5);   // Länger als der Job im Fehlerfall läuft. Siehe blockedSignal.job.xml


//    @Test public void test() throws Exception {
//        if (OperatingSystem.isUnix) {   // Sollte der Windows-Scheduler die Cygwin-bash starten, wenn das Skript mit "#!/usr/bin/env bash" beginnt?
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
