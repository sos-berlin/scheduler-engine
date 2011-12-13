package com.sos.scheduler.engine.test;

import static com.google.common.base.Preconditions.checkState;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.toArray;
import static com.sos.scheduler.engine.kernel.util.Files.makeTemporaryDirectory;
import static com.sos.scheduler.engine.kernel.util.Files.tryRemoveDirectoryRecursivly;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventHandlerFailedEvent;
import com.sos.scheduler.engine.eventbus.EventSubscriberAdaptingEventSubscription;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.log.ErrorLogEvent;
import com.sos.scheduler.engine.kernel.settings.SettingName;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Files;
import com.sos.scheduler.engine.kernel.util.Hostware;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.main.CppBinaries;
import com.sos.scheduler.engine.main.CppBinary;
import com.sos.scheduler.engine.main.SchedulerState;
import com.sos.scheduler.engine.test.binary.TestCppBinaries;

public class TestSchedulerController extends DelegatingSchedulerController implements EventHandlerAnnotated {
    private static final Logger logger = Logger.getLogger(TestSchedulerController.class);
    private static final String workDirectoryPropertyName = "com.sos.scheduler.engine.test.directory";
    public static final Time shortTimeout = Time.of(10);

    private final List<Runnable> closingRunnables = new ArrayList<Runnable>();
    private final SchedulerEventBus eventBus = getEventBus();
    private final Thread thread = Thread.currentThread();
    private final Environment environment;
    private boolean terminateOnError = true;
    private String logCategories = "";
    private Scheduler scheduler = null;

    public TestSchedulerController(Class<?> testClass, ResourcePath configurationResourcePath) {
        File directory = workDirectory(testClass);
        environment = new Environment(configurationResourcePath, directory);
        setSettings(Settings.of(SettingName.jobJavaClassPath, System.getProperty("java.class.path")));
    }

    private File workDirectory(Class<?> testClass) {
        String p = System.getProperty(workDirectoryPropertyName);
        if (p != null) {
            File result = new File(p, testClass.getName());
            makeCleanDirectory(result);
            return result;
        } else {
            final File result = makeTemporaryDirectory();
            closingRunnables.add(new Runnable() {
                @Override public void run() { tryRemoveDirectoryRecursivly(result); }
            });
            return result;
        }
    }

    private static void makeCleanDirectory(File directory) {
        Files.makeDirectory(directory.getParentFile());
        Files.makeDirectory(directory);
        Files.removeDirectoryContentRecursivly(directory);
    }

    @Override public final void close() {
        try {
            getDelegate().close();
        } finally {
            for (Runnable r: closingRunnables) r.run();
        }
    }

    /** Bricht den Test mit Fehler ab, wenn ein {@link com.sos.scheduler.engine.kernel.log.ErrorLogEvent} ausgelöst worden ist. */
    public final void setTerminateOnError(boolean o) {
        getDelegate().checkIsNotStarted();
        terminateOnError = o;
    }

    public final void setLogCategories(String o) {
        getDelegate().checkIsNotStarted();
        logCategories = o;
    }

    @Deprecated
    public final void subscribeEvents(EventSubscriber s) {
        eventBus.registerHot(new EventSubscriberAdaptingEventSubscription(s));
    }

    /** @param timeout Wenn ab Bereitschaft des Schedulers mehr Zeit vergeht, wird eine Exception ausgelöst */
    public final void runScheduler(Time timeout, String... args) {
        activateScheduler(args);
        waitForTermination(timeout);
    }

    @Deprecated  // Statt den Scheduler nach Frist zu beenden, soll der Test das bitte selbst machen, sobald alles okay ist. Zschimmer 8.11.2011
    public final void runSchedulerAndTerminate(Time timeout, String... args) {
        startScheduler(args);
        waitUntilSchedulerIsActive();
        try {
            waitForTermination(timeout);
        } catch (SchedulerRunningAfterTimeoutException x) {
            logger.warn("runSchedulerAndTerminate():"+ x.getMessage());
        }
    }

    /** Startet den Scheduler und wartet, bis er aktiv ist. */
    public final void activateScheduler(String... args) {
        startScheduler(args);
        waitUntilSchedulerIsActive();
    }

    @Override public final void startScheduler(String... args) {
        registerEventHandler(this);
        prepare();
        Iterable<String> allArgs = concat(environment.standardArgs(cppBinaries(), logCategories), Arrays.asList(args));
        getDelegate().startScheduler(toArray(allArgs, String.class));
    }

    private void prepare() {
        environment.prepare();
        getDelegate().loadModule(cppBinaries().file(CppBinary.moduleFilename));
    }

    public final Scheduler scheduler() {
        if (scheduler == null) {
            automaticStart();
            waitUntilSchedulerIsActive();
        }
        return scheduler;
    }

    private void automaticStart() {
        if (!getDelegate().isStarted()) {
            checkState(Thread.currentThread() == thread, "TestSchedulerController.automaticStart() must be called in constructing thread");
            startScheduler();
        }
    }

    /** Wartet, bis das Objekt {@link Scheduler} verfügbar ist. */
    public final void waitUntilSchedulerIsActive() {
        Scheduler previous = scheduler;
        scheduler = getDelegate().waitUntilSchedulerState(SchedulerState.active);
        if (scheduler == null) throw new RuntimeException("Scheduler aborted before startup");
        if (previous == null && terminateOnError) checkForErrorLogLine();
    }

    public final void waitForTermination(Time timeout) {
        automaticStart();
        boolean ok = tryWaitForTermination(timeout);
        if (!ok) throw new SchedulerRunningAfterTimeoutException(timeout);
    }

    private void checkForErrorLogLine() {
        // TODO Prefix_log.last() liefert nur für info die Start-Meldung SCHEDULER-900. Wo sind die anderen Meldungen?
//        String lastErrorLine = scheduler.log().lastByLevel(SchedulerLogLevel.error);
//        if (!lastErrorLine.isEmpty())
//            throw throwErrorLogException(lastErrorLine);
    }

    @EventHandler public final void handleEvent(ErrorLogEvent e) {
        if (terminateOnError)
            terminateAfterException(new RuntimeException("Test terminated after error log line: "+ e.getMessage()));
    }

    @EventHandler @HotEventHandler  // Beide, weil das EventHandlerFailedEvent wird nur innerhalb von Hot- oder ColdEventBus veröffentlicht wird.
    public final void handleEvent(EventHandlerFailedEvent e) {
        if (terminateOnError)
            terminateAfterException(e.getThrowable());
    }

    /** Eine Exception in {@code runnable} beendet den Scheduler. */
    public final Thread newThread(Runnable runnable) {
        ThreadTerminationHandler h = new ThreadTerminationHandler() {
            @Override public void onThreadTermination(@Nullable Throwable t) {
                if (t != null) terminateAfterException(t);
            }
        };
        return new Thread(new TestThreadRunnable(runnable, h), runnable.toString());
    }

    /** @throws IllegalStateException, wenn nach {#startScheduler} aufgerufen. Das wird vorsichthalber erzwungen, weil sonst Events verlorengehen können. */
    public final EventPipe newEventPipe() {
        getDelegate().checkIsNotStarted();
        final EventPipe result = new EventPipe();
        registerEventHandler(result);
        return result;
    }

    private void registerEventHandler(final EventHandlerAnnotated o) {
        eventBus.registerAnnotated(o);
        closingRunnables.add(new Runnable() {
            @Override public void run() { eventBus.unregisterAnnotated(o); }
        });
    }

    public final void useDatabase() {
        File databaseFile = new File(environment.directory(), "scheduler-database");
        String dbName = Hostware.h2DatabasePath(databaseFile);
        setSettings(Settings.of(SettingName.dbName, dbName));
    }

    public final Environment environment() {
        return environment;
    }

    public final CppBinaries cppBinaries() {
        return TestCppBinaries.cppBinaries();
    }

    /** @param testClass Test-Klasse, für Benennung des Scheduler-Arbeitsverzeichnisses und Ort der Konfigurationsresourcen. */
    public static TestSchedulerController of(Class<?> testClass) {
        return new TestSchedulerController(testClass, new ResourcePath(testClass.getPackage()));
    }
}
