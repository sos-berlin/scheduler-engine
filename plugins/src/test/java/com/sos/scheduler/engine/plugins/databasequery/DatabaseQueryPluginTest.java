package com.sos.scheduler.engine.plugins.databasequery;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsString;
import org.junit.Ignore;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;

public class DatabaseQueryPluginTest extends SchedulerTest {
    public DatabaseQueryPluginTest() throws Exception {
        controller().startScheduler("-e");
        waitForTaskTermination();
    }


    private void waitForTaskTermination() throws Exception {
    	controller().waitUntilSchedulerIsRunning();
        Thread.sleep(10*1000);   // TODO Besser TerminatedTaskEvent abwarten, daber das haben wir noch nicht.
    }


    @Ignore
//  @Test
//  @TODO Dieser Test sollte sobald wie möglich wieder aktiviert werden. Derzeit gibt es Probleme mit dem
//  Datenbankzugriff.
//  2011-07-22 12:10:47.327 [info]   SCHEDULER-900  Scheduler 1.3.12-beta-1a-SNAPSHOT (Debug) is starting with C:\Users\schaedi\AppData\Local\Temp\sos152924468595739875.tmp\scheduler.xml, pid=2928
//  2011-07-22 12:10:47.327 [info]   (Database) SCHEDULER-907  Opening database: jdbc  -id=spooler -class=org.h2.Driver  jdbc:h2:./scheduler-database
//  2011-07-22 12:10:47.358 [ERROR]  SCHEDULER-303  Problem with database: SOS-1244  Server-Problem: Client nicht ermittelbar (obj_client() versagt). M÷glicherweise ein nicht fileserver-kompatibler Dateityp [**sos_static**] [jdbc  -id=spooler -class=org.h2.Driver  jdbc:h2:./scheduler-database] [sos::scheduler::database::Database::open]
//  2011-07-22 12:10:47.374 [info]   (Database) SCHEDULER-907  Opening database: jdbc  -id=spooler -class=org.h2.Driver  jdbc:h2:./scheduler-database
//  2011-07-22 12:10:47.374 [WARN]   SOS-1244  Server-Problem: Client nicht ermittelbar (obj_client() versagt). M÷glicherweise ein nicht fileserver-kompatibler Dateityp [**sos_static**] [jdbc  -id=spooler -class=org.h2.Driver  jdbc:h2:./scheduler-database]
//  2011-07-22 12:10:47.374 [WARN]   SCHEDULER-958  Waiting 20 seconds before reopening the database
//  (letzten fünf Zeilen wiederholen sich immer wieder)    
    public void testShowTaskHistory() throws Exception {
        String result = execute("<showTaskHistory/>");
        System.err.println(result);
        assertThat(result, containsString("</myResult>"));
        assertThat(result, containsString("<row "));
        assertThat(result, containsString(" job="));
    }


    private String execute(String subcommandXml) throws Exception {
        String commandXml =
                "<plugin.command plugin_class='" + DatabaseQueryPlugin.class.getName() + "'>" +
                subcommandXml +
                "</plugin.command>";
        return scheduler().executeXml(commandXml);
    }
}
