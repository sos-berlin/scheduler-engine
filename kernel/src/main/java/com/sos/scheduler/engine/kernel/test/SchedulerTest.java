package com.sos.scheduler.engine.kernel.test;

import java.io.File;

import org.junit.After;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.settings.database.DatabaseSettings;
import com.sos.scheduler.engine.kernel.settings.database.DefaultDatabaseSettings;
import com.sos.scheduler.engine.kernel.util.Hostware;
import com.sos.scheduler.engine.kernel.util.Time;

public abstract class SchedulerTest implements EventHandlerAnnotated {
    public static final Time shortTimeout = TestSchedulerController.shortTimeout;

    private final TestSchedulerController controller = TestSchedulerController.of(getClass().getPackage());

    protected SchedulerTest() {
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

    protected Settings temporaryDatabaseSettings() {
        return new DefaultSettings() {
            @Override public DatabaseSettings getDatabaseSettings() {
                return new DefaultDatabaseSettings() {
                    @Override public String getHostwarePathOrNull() {
                        File databaseFile = new File(controller.environment().directory(), "scheduler-database");
                        return Hostware.h2DatabasePath(databaseFile);
                    }
                };
            }
        };
    }
}
