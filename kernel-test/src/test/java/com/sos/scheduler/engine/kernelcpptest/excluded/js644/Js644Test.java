package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import static com.sos.scheduler.engine.kernelcpptest.excluded.js644.Configuration.jobFilenames;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Collection;
import java.util.Iterator;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

import org.junit.ClassRule;
import org.junit.Test;
import org.junit.rules.TestRule;

import com.google.common.base.Charsets;
import com.google.common.collect.Iterators;
import com.google.common.io.Files;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.test.junit.SlowTestRule;
import com.sos.scheduler.engine.kernel.util.Time;

public class Js644Test extends SchedulerTest {
    private static final Time orderTimeout = Time.of(10);
    private static final Charset encoding = Charsets.UTF_8;
    @ClassRule public static TestRule slowTestRule = SlowTestRule.singleton;

    private final BlockingQueue<Boolean> eventReceivedQueue = new ArrayBlockingQueue<Boolean>(1);


    @Test public void test() throws Exception {
        strictSubscribeEvents(new MyEventSubscriber());
        startScheduler("-e");
        Thread fileModifierThread = new FileModifierThread(getDirectory());
        fileModifierThread.start();
        try {
            for (int i = 0; i < 1000; i++) {
                eventReceivedQueue.poll(orderTimeout.getMillis(), TimeUnit.MILLISECONDS);
            }
        } finally {
            fileModifierThread.interrupt();
        }
    }


    private class MyEventSubscriber implements EventSubscriber {
        @Override public void onEvent(Event event) throws InterruptedException {
            if (event instanceof OrderFinishedEvent) {
                eventReceivedQueue.put(true);
            }
        }
    }


    private static class FileModifierThread extends Thread {
        private final FileModifier fileModifier;
        
        private FileModifierThread(File directory) {
            fileModifier = new FileModifier(directory, jobFilenames);
        }

        @Override public void run() {
            try {
                int pause = 4000 / fileModifier.getFileCount() + 1;   // Mindestens 2s Pause, damit Scheduler Datei als stabil ansieht.
                while(true) {
                    fileModifier.modifyNext();
                    Thread.sleep(pause);
                }
            } catch (InterruptedException x) {
                // Ende
            }
        }
    }

    
    private static class FileModifier {
        private final File directory;
        private final Collection<String> files;
        private Iterator<String> iterator = Iterators.emptyIterator();

        private FileModifier(File directory, Collection<String> files) {
            this.directory = directory;
            this.files = files;
        }
        
        private void modifyNext() {
            if (!iterator.hasNext())
                iterator = files.iterator();
            modifyFile(iterator.next());
        }


        private void modifyFile(String filename) {
            try {
                File f = new File(directory, filename);
                assert f.exists();
                Files.append("<!-- -->\n", f, encoding);
            } catch (IOException x) { throw new RuntimeException(x); }
        }

        private int getFileCount() {
            return files.size();
        }
    }
}
