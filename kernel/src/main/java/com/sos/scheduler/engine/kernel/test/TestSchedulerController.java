package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Strings.isNullOrEmpty;
import static com.google.common.base.Throwables.propagate;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.toArray;
import static com.sos.scheduler.engine.kernel.util.Files.makeDirectory;

import java.io.File;
import java.util.Arrays;

import org.apache.log4j.Logger;
import org.junit.rules.TemporaryFolder;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.main.SchedulerController;
import com.sos.scheduler.engine.kernel.main.SchedulerThreadController;
import com.sos.scheduler.engine.kernel.util.Lazy;
import com.sos.scheduler.engine.kernel.util.Time;

public class TestSchedulerController implements SchedulerController {
    private static final Logger logger = Logger.getLogger(TestSchedulerController.class);

    private final TestSchedulerCppModule cppModule = new TestSchedulerCppModule();
    private final SchedulerThreadController delegated;
    private final Environment environment;
    private Scheduler scheduler = null;

    public TestSchedulerController(Package pack, Lazy<File> testDirectory) {
        delegated = new SchedulerThreadController();
        environment = new Environment(cppModule, pack, testDirectory);
    }

    public final void strictSubscribeEvents() {
        strictSubscribeEvents(EventSubscriber.empty);
    }

    public final void strictSubscribeEvents(EventSubscriber s) {
        delegated.subscribeEvents(new StrictEventSubscriber(s));
    }

    @Override public void subscribeEvents(EventSubscriber s) {
        delegated.subscribeEvents(s);
    }

    public final void runScheduler(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsRunning();
        waitForTermination(timeout);
    }

    public final void startScheduler(String... args) {
        delegated.loadModule(cppModule.moduleFile());
        environment.start();
        Iterable<String> allArgs = concat(environment.standardArgs(), Arrays.asList(args));
        delegated.startScheduler(toArray(allArgs, String.class));
    }

    public final Scheduler scheduler() {
        if (scheduler == null)
            waitUntilSchedulerIsRunning();
        assert scheduler != null;
        return scheduler;
    }

    public final Scheduler waitUntilSchedulerIsRunning() {
        scheduler = delegated.waitUntilSchedulerIsRunning();
        return scheduler;
    }

    @Override public void terminateScheduler() {
        delegated.terminateScheduler();
    }

    @Override public void terminateAfterException(Throwable x) {
        delegated.terminateAfterException(x);
    }

    @Override public void terminateAndWait() {
        delegated.terminateAndWait();
    }

    @Override public final void waitForTermination(Time timeout) {
        delegated.waitForTermination(timeout);
    }

    @Override public int exitCode() {
        return delegated.exitCode();
    }

    public final void close() {
        try {
            delegated.terminateAndWait();
        }
        catch (Throwable x) {
            logger.error(TestSchedulerController.class.getName() + ".close(): " + x, x);
            propagate(x);
        }
    }

    public final File directory() {
        return environment.getDirectory();
    }

    public static TestSchedulerController of(Package pack, final TemporaryFolder temporaryFolder) {
        Lazy<File> dirLazy = new Lazy<File>() {
            public File compute() {
                assert temporaryFolder.getRoot() != null : "TemporaryFolder.@Before has not been executed?";
                String prop = System.getProperty("test.scheduler.dir");
                if (isNullOrEmpty(prop))
                    return temporaryFolder.getRoot();
                else {
                    File result = new File(prop);
                    makeDirectory(result);
                    return result;
                }
            }
        };
        return new TestSchedulerController(pack, dirLazy);
    }
}
