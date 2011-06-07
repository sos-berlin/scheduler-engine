package com.sos.scheduler.engine.plugins.guicommand;

import com.sos.scheduler.engine.kernel.database.DatabaseConfiguration;
import com.sos.scheduler.engine.kernel.plugin.PlugInCommandCommand;
import org.w3c.dom.Element;
import org.junit.Ignore;
import org.junit.Test;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


public class GUICommandPluginTest {
    private final String dbUrl = "";
//    private final DatabaseConfiguration dbConfig = new DatabaseConfiguration(dbUrl, "SCHEDULER_JOB_HISTORY");

    @Ignore @Test public void testExecuteCommand() {
        String commandXml = "<plugin.command class='" + GUICommandPlugin.class.getName() + "'><test/></plugin.command>";
        Element commandElement = loadXml(commandXml).getDocumentElement();
        PlugInCommandCommand command = new PlugInCommandCommand(GUICommandPlugin.class.getName(), commandElement);
        //TODO GuidCommandPlugun braucht ein DatabaseSubsystemMock.
//        GUICommandPlugin plugin = new GUICommandPlugin(dbConfig);
//        PlugInCommandResult r = plugin.executeCommand(command);
//        assertThat(r, notNullValue());
    }
}
