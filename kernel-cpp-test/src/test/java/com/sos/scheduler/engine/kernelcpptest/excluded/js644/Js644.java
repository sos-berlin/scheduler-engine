package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import com.google.common.base.Function;
import com.google.common.base.Charsets;
import com.google.common.io.Files;
import com.google.common.collect.Iterators;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;
import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.*;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import org.apache.log4j.Logger;
import org.junit.Test;
import static com.google.common.collect.Collections2.transform;
import static com.google.common.collect.Iterables.concat;
import static java.util.Arrays.asList;


public class Js644 extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(Js644.class);
    private static final Time orderTimeout = Time.of(10);
    private static final List<String> jobPaths = asList("exercise10a", "exercise10b", "exercise10c");
    private static final Collection<String> jobFilenames = transform(jobPaths, append(".job.xml"));
    private static final Iterable<String> configFilenames = concat(jobFilenames, asList("exercise10.job_chain.xml", "exercise10,1.order.xml"));
    private static final Charset encoding = Charsets.UTF_8;

    private final BlockingQueue<Boolean> eventReceivedQueue = new ArrayBlockingQueue<Boolean>(1);


    public Js644() {
        super(configFilenames);
    }


    private static Function<String,String> append(final String appendix) {
        return new Function<String,String>() {
            @Override public String apply(String a) {
                return a + appendix;
            }
        };
    }

    
    @Test public void test() throws Exception {
        strictSubscribeEvents(new MyEventSubscriber());
        startScheduler("-e");
        getScheduler().executeXml("<modify_job job='" + jobPaths.get(0) + "' cmd='stop'/>");
        //for (String j: jobPaths) logger.info(getScheduler().executeXml("<job.why job='" + j + "'/>"));    //TODO Test fehlt
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
