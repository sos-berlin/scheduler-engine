package com.sos.scheduler.engine.kernel.command;

import org.junit.Test;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;
import static java.util.Arrays.asList;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

/**
 *
 * @author Joacim Zschimmer
 */
public class CommandDispatcherTest {
    private CommandDispatcher commandDispatcher = commandDispatcher();


    private static CommandDispatcher commandDispatcher() {
        Iterable<CommandHandler> commandHandlers = asList(
            ACommandExecutor.singleton,
            ACommandXmlParser.singleton,
            AResultXmlizer.singleton,
            BCommandExecutor.singleton,
            BCommandXmlParser.singleton,
            BResultXmlizer.singleton);
        return new CommandDispatcher(commandHandlers);
    }


    @Test public void testExecuteXml_A() {
        String resultXml = commandDispatcher.executeXml("<a value1='VALUE 1'/>");
        Element e = loadXml(resultXml).getDocumentElement();
        assertThat(e.getNodeName(), equalTo("aResult"));
        assertThat(e.getAttribute("value1"), equalTo("VALUE 1"));
    }


    @Test public void testExecuteXml_B() {
        String resultXml = commandDispatcher.executeXml("<b value2='VALUE 2'/>");
        Element e = loadXml(resultXml).getDocumentElement();
        assertThat(e.getNodeName(), equalTo("bResult"));
        assertThat(e.getAttribute("value2"), equalTo("VALUE 2"));
    }
}
