package com.sos.scheduler.engine.plugins.databasequery;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsString;

import java.io.File;

import org.apache.log4j.Logger;
import org.junit.ClassRule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.settings.database.DatabaseSettings;
import com.sos.scheduler.engine.kernel.settings.database.DefaultDatabaseSettings;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;

public final class DatabaseQueryPluginTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(DatabaseQueryPluginTest.class);
    @ClassRule public static final TemporaryFolder tempDir = new TemporaryFolder();

    public DatabaseQueryPluginTest() throws Exception {
        super(settings());
        controller().startScheduler();
        waitForTaskTermination();
    }

    private static Settings settings() {
        return new DefaultSettings() {
            @Override public DatabaseSettings getDatabaseSettings() {
                return new DefaultDatabaseSettings() {
                    @Override public String getHostwarePathOrNull() {
                        File databaseFile = new File(tempDir.getRoot(), "scheduler-database");
                        return "jdbc -class="+org.h2.Driver.class.getName()+" jdbc:h2:"+databaseFile;
                    }
                };
            }
        };
    }

    private void waitForTaskTermination() throws Exception {
    	controller().waitUntilSchedulerIsRunning();
        Thread.sleep(10*1000);   // TODO Besser TaskTerminatedEvent abwarten, aber das haben wir noch nicht.
    }

    @Test
    public void testShowTaskHistory() throws Exception {
        String result = execute("<showTaskHistory/>");
        assertThat(result, containsString("</myResult>"));
        assertThat(result, containsString("<row "));
        assertThat(result, containsString(" job="));
    }

    private String execute(String subcommandXml) throws Exception {
        String commandXml =
                "<plugin.command plugin_class='" + DatabaseQueryPlugin.class.getName() + "'>" +
                    subcommandXml +
                "</plugin.command>";
        String result = scheduler().executeXml(commandXml);
        logger.debug(result);
        return result;
    }
}
