package com.sos.scheduler.engine.test;

import com.google.common.base.Predicate;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.common.system.Files;
import com.sos.scheduler.engine.common.time.Time;
import com.sos.scheduler.engine.data.log.ErrorLogEvent;
import com.sos.scheduler.engine.data.log.SchedulerLogLevel;
import com.sos.scheduler.engine.eventbus.*;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.settings.SettingName;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Hostware;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import com.sos.scheduler.engine.main.CppBinaries;
import com.sos.scheduler.engine.main.CppBinary;
import com.sos.scheduler.engine.main.SchedulerState;
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode;
import com.sos.scheduler.engine.test.binary.TestCppBinaries;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.annotation.Nullable;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import static com.google.common.base.Preconditions.checkState;
import static com.google.common.base.Strings.nullToEmpty;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.toArray;
import static com.sos.scheduler.engine.common.system.Files.makeTemporaryDirectory;
import static com.sos.scheduler.engine.common.system.Files.tryRemoveDirectoryRecursivly;
import static java.util.Arrays.asList;

public class TestSchedulerController extends DelegatingSchedulerController implements EventHandlerAnnotated {
    private static final Logger logger = LoggerFactory.getLogger(TestSchedulerController.class);
    private static final String workDirectoryPropertyName = "com.sos.scheduler.engine.test.directory";
    public static final Time shortTimeout = Time.of(15);
    private static final AtomicInteger instanceCount = new AtomicInteger(0);

    private final List<Runnable> closingRunnables = new ArrayList<Runnable>();
    private final SchedulerEventBus eventBus = getEventBus();
    private final Thread thread = Thread.currentThread();
    private final Environment environment;
    private final Predicate<ErrorLogEvent> expectedErrorLogEventPredicate;
    private final CppBinariesDebugMode debugMode;

    private boolean terminateOnError = true;
    private boolean isPrepared = false;
    private String logCategories = "";
    private Scheduler _scheduler = null;   // Unterstrich, damit IntelliJ-Scala-Plugin scheduler() findet, Zschimmer 9.12.2011

    public TestSchedulerController(Class<?> testClass, ResourcePath configurationResourcePath,
            @Nullable ImmutableMap<String,String> nameMap,
            @Nullable ResourceToFileTransformer fileTransformer,
            Predicate<ErrorLogEvent> expectedErrorLogEventPredicate,
            CppBinariesDebugMode debugMode) {
        super(testClass.getName());
        logger.debug(testClass.getName());
        instanceCount.addAndGet(1);
        environment = new Environment(configurationResourcePath, workDirectory(testClass), nameMap, fileTransformer);
        this.expectedErrorLogEventPredicate = expectedErrorLogEventPredicate;
        this.debugMode = debugMode;
        setSettings(Settings.of(SettingName.jobJavaClasspath, System.getProperty("java.class.path")));
    }

    private File workDirectory(Class<?> testClass) {
        String p = System.getProperty(workDirectoryPropertyName);
        if (p != null) {
            new File(p).mkdir();    // Das Unterverzeichnis failsafe-reports wird verzögert angelegt. Dem kommen wir zuvor.
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
        //Files.makeDirectory(directory.getParentFile());
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

    /** Bricht den Test mit Fehler ab, wenn ein {@link com.sos.scheduler.engine.data.log.ErrorLogEvent} ausgelöst worden ist. */
    public final void setTerminateOnError(boolean o) {
        getDelegate().checkIsNotStarted();
        terminateOnError = o;
    }

    public final void setLogCategories(String o) {
        getDelegate().checkIsNotStarted();
        logCategories = o;
    }

    /** @param timeout Wenn ab Bereitschaft des Schedulers mehr Zeit vergeht, wird eine Exception ausgelöst */
    public final void runScheduler(Time timeout, String... args) {
        activateScheduler(args);
        waitForTermination(timeout);
    }

    /** Startet den Scheduler und wartet, bis er aktiv ist. */
    public final void activateScheduler(String... args) {
        startScheduler(args);
        waitUntilSchedulerIsActive();
    }

    @Override public final void startScheduler(String... args) {
        prepare();
        String extraOptions = nullToEmpty(System.getProperty(TestSchedulerController.class.getName() +".options"));
        Iterable<String> allArgs = concat(
                environment.standardArgs(cppBinaries(), logCategories),
                Splitter.on(",").omitEmptyStrings().split(extraOptions),
                asList(args));
        getDelegate().startScheduler(toArray(allArgs, String.class));
    }

    public void prepare() {
        if (!isPrepared) {
            registerEventHandler(this);
            environment.prepare();
            getDelegate().loadModule(cppBinaries().file(CppBinary.moduleFilename));
            isPrepared = true;
        }
    }

    public final Scheduler scheduler() {
        if (_scheduler == null) {
            automaticStart();
            waitUntilSchedulerIsActive();
        }
        return _scheduler;
    }

    /** Wartet, bis das Objekt {@link Scheduler} verfügbar ist. */
    public final void waitUntilSchedulerIsActive() {
        Scheduler previous = _scheduler;
        _scheduler = getDelegate().waitUntilSchedulerState(SchedulerState.active);
        if (_scheduler == null) throw new RuntimeException("Scheduler aborted before startup");
        if (previous == null && terminateOnError) checkForErrorLogLine();
    }

    public final void waitForTermination(Time timeout) {
        boolean ok = tryWaitForTermination(timeout);
        if (!ok) throw new SchedulerRunningAfterTimeoutException(timeout);
    }

    private void automaticStart() {
        if (!getDelegate().isStarted()) {
            checkState(Thread.currentThread() == thread, "TestSchedulerController.automaticStart() must be called in constructing thread");
            throw new RuntimeException("JobScheduler is not active yet");
//            startScheduler();
        }
    }

    private void checkForErrorLogLine() {
        String lastErrorLine = _scheduler.injector().getInstance(PrefixLog.class).lastByLevel(SchedulerLogLevel.error);
        if (!lastErrorLine.isEmpty())
            throw new RuntimeException("Test terminated after error log line: "+ lastErrorLine);
    }

    @EventHandler public final void handleEvent(ErrorLogEvent e) {
        if (!expectedErrorLogEventPredicate.apply(e) && terminateOnError)
            terminateAfterException(new RuntimeException("Test terminated after error log line: "+ e.getLine()));
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

    /** Rechtzeitig aufrufen, dass kein Event verloren geht. */
    public final EventPipe newEventPipe() {
        EventPipe result = new EventPipe(shortTimeout);
        registerEventHandler(result);
        return result;
    }

    private void registerEventHandler(final EventHandlerAnnotated o) {
        eventBus.registerAnnotated(o);
        closingRunnables.add(new Runnable() {
            @Override public void run() { eventBus.unregisterAnnotated(o); }
        });
    }

    public final boolean isStarted() {
        return getDelegate().isStarted();
    }

    public final void useDatabase() {
        String dbName = Hostware.h2DatabasePath("mem:scheduler-"+instanceCount);
        setSettings(Settings.of(SettingName.dbName, dbName));
    }

    public final Environment environment() {
        return environment;
    }

    public final CppBinaries cppBinaries() {
        return TestCppBinaries.cppBinaries(debugMode);
    }

    /** @param testClass Test-Klasse, für Benennung des Scheduler-Arbeitsverzeichnisses und Ort der Konfigurationsresourcen. */
    public static TestSchedulerController of(Class<?> testClass) {
        return builder(testClass).build();
    }

    /** @param testClass Test-Klasse, für Benennung des Scheduler-Arbeitsverzeichnisses und Ort der Konfigurationsresourcen. */
    public static TestSchedulerControllerBuilder builder(Class<?> testClass) {
        return new TestSchedulerControllerBuilder(testClass);
    }
}
