package com.sos.scheduler.engine.plugins.guicommand;

import com.sos.scheduler.engine.kernel.database.DatabaseConfiguration;
import com.sos.scheduler.engine.kernel.plugin.PlugInCommandCommand;
import com.sos.scheduler.engine.kernel.plugin.PlugInCommandResult;
import org.w3c.dom.Element;
import org.junit.Test;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;


public class GUICommandPluginTest {
    private final String dbUrl = "";
    private final DatabaseConfiguration dbConfig = new DatabaseConfiguration(dbUrl, "SCHEDULER_JOB_HISTORY");

    @Test public void testExecuteCommand() {
        String commandXml = "<plugin.command class='" + GUICommandPlugin.class.getName() + "'><test/></plugin.command>";
        Element commandElement = loadXml(commandXml).getDocumentElement();
        PlugInCommandCommand command = new PlugInCommandCommand(GUICommandPlugin.class.getName(), commandElement);
        GUICommandPlugin plugin = new GUICommandPlugin(dbConfig);
        PlugInCommandResult r = plugin.executeCommand(command);
        assertThat(r, notNullValue());
    }
}
