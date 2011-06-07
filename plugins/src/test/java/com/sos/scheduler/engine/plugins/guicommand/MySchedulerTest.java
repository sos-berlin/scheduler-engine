package com.sos.scheduler.engine.plugins.guicommand;

import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import org.junit.Ignore;
import org.junit.Test;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;


public abstract class MySchedulerTest extends SchedulerTest {
    // Abstrakt, weil der Scheduler nur einmal pro Prozess aufgerufen werden kann. Also nur eine @Test-Methode!
    

    public MySchedulerTest() {
        startScheduler("-e");
    }


    protected final String executeGuiCommand(String guiXml) throws Exception {
        String commandXml =
                "<plugin.command plugin_class='" + GUICommandPlugin.class.getName() + "'>" +
                guiXml +
                "</plugin.command>";
        return getScheduler().executeXml(commandXml);
    }
}
