package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Strings.isNullOrEmpty;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.toArray;

import java.io.File;
import java.util.Arrays;

import org.apache.log4j.Logger;
import org.junit.After;
import org.junit.rules.TemporaryFolder;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.main.SchedulerThread;
import com.sos.scheduler.engine.kernel.util.Time;

public class SchedulerTestDriver {
    public static final Time shortTimeout = Time.of(10);
    private static final Logger logger = Logger.getLogger(SchedulerTestDriver.class);

    private final SchedulerController controller;
    private Scheduler scheduler = null;
    private final Environment env;

    public SchedulerTestDriver(Package pack, File testDirectory) {
        controller = new SchedulerThread();
        env = new Environment(pack, testDirectory);
    }

    public final void strictSubscribeEvents() {
        strictSubscribeEvents(EventSubscriber.empty);
    }

    public final void strictSubscribeEvents(EventSubscriber s) {
        controller.subscribeEvents(new StrictEventSubscriber(s));
    }

    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    public final void startScheduler(String... args) {
        Iterable<String> allArgs = concat(env.standardArgs(), Arrays.asList(args));
        controller.startScheduler(toArray(allArgs, String.class));
    }

    public final void waitUntilSchedulerIsRunning() {
        scheduler = controller.waitUntilSchedulerIsRunning();
    }

    public final Scheduler scheduler() {
        if (scheduler == null)
            waitUntilSchedulerIsRunning();
        assert scheduler != null;
        return scheduler;
    }

    public final void waitForTermination(Time timeout) {
        controller.waitForTermination(timeout);
    }

    @After public final void terminateAndCleanUp() throws Throwable {
        try {
            controller.terminateAndWait();
        }
        catch (Throwable x) {
            logger.error(SchedulerTestDriver.class.getName() + " @After: " + x, x);
            throw x;
        }
    }

    public final File directory() {
        return env.getDirectory();
    }

    public final SchedulerController controller() {
        return controller;
    }

    public static SchedulerTestDriver of(Package pack, TemporaryFolder f) {
        assert f.getRoot() != null : "TemporaryFolder.@Before has not been executed?";
        String d = System.getProperty("test.scheduler.dir");
        File dir = isNullOrEmpty(d)? f.getRoot() : new File(d);
        return new SchedulerTestDriver(pack, dir);
    }

//    static interface Dir extends Closeable {
//        File file();
//    }
//
//    static class NamedDir implements Dir {
//        private final File dir;
//        NamedDir(File dir) { this.dir = dir; }
//        @Override public File file() { return dir; }
//        @Override public void close() {}
//    }
//
//    static class TemporaryDir implements Dir {
//        private final File dir = makeTemporaryDirectory();
//        @Override public File file() { return dir; }
//        @Override public void close() throws IOException {
//            removeDirectoryRecursivlyFollowingLinks(dir);   // Wir haben keine Links
//        }
//    }
}
