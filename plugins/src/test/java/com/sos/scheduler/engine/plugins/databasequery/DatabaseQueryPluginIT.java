package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.configuration.DefaultDatabaseConfiguration;
import com.sos.scheduler.engine.test.configuration.TestConfigurationBuilder;
import org.joda.time.Duration;
import org.junit.Test;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsString;

public final class DatabaseQueryPluginIT extends SchedulerTest {
    private final Duration timeout = Duration.standardSeconds(20);

    public DatabaseQueryPluginIT() {
        super(new TestConfigurationBuilder(DatabaseQueryPluginIT.class)
                .logCategories("java.stackTrace-")  // Exceptions wegen fehlender Datenbanktabellen wollen wir nicht sehen.
                .database(DefaultDatabaseConfiguration.forJava())
                .build());
    }

    @Test public void testShowTaskHistory() throws Exception {
        controller().activateScheduler();
        controller().waitForTermination(timeout);
    }

    @EventHandler public void handleEvent(TaskEndedEvent e) {
        try {
            String result = execute("<showTaskHistory/>");
            assertThat(result, containsString("</myResult>"));
            assertThat(result, containsString("<row "));
            assertThat(result, containsString(" job="));
        } finally {
            controller().terminateScheduler();
        }
    }

    private String execute(String subcommandXml) {
        String commandXml =
                "<plugin.command plugin_class='" + DatabaseQueryPlugin.class.getName() + "'>" +
                    subcommandXml +
                "</plugin.command>";
        return scheduler().executeXml(commandXml);
    }
}
