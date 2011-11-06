package com.sos.scheduler.engine.kernel.test;

import java.io.File;

import org.junit.After;
import org.junit.ClassRule;
import org.junit.rules.TemporaryFolder;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.settings.database.DatabaseSettings;
import com.sos.scheduler.engine.kernel.settings.database.DefaultDatabaseSettings;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest implements EventHandlerAnnotated {
    public static final Time shortTimeout = TestSchedulerController.shortTimeout;
    @ClassRule public static final TemporaryFolder temporaryDirectory = new TemporaryFolder();

    private final TestSchedulerController controller;

    protected SchedulerTest() {
        this(DefaultSettings.singleton);
    }

    protected SchedulerTest(Settings settings) {
        controller = TestSchedulerController.of(getClass().getPackage(), settings);
        controller.subscribeForAnnotatedEventHandlers(this);
    }

    @After public final void schedulerTestClose() {
        controller.close();
    }

    public final TestSchedulerController controller() {
        return controller;
    }

    /** Zur Bequemlichkeit; dasselbe wie {@link com.sos.scheduler.engine.kernel.test.TestSchedulerController#scheduler()}. */
    protected final Scheduler scheduler() {
        return controller().scheduler();
    }

    protected static Settings temporaryDatabaseSettings() {
        return new DefaultSettings() {
            @Override public DatabaseSettings getDatabaseSettings() {
                return new DefaultDatabaseSettings() {
                    @Override public String getHostwarePathOrNull() {
                        File databaseFile = new File(temporaryDirectory.getRoot(), "scheduler-database");
                        return "jdbc -class="+org.h2.Driver.class.getName()+" jdbc:h2:"+databaseFile;
                    }
                };
            }
        };
    }
}
