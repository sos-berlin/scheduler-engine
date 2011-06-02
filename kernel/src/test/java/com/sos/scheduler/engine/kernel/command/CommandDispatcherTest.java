package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Document;
import java.util.Collections;
import java.util.Arrays;
import org.junit.Test;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

/**
 *
 * @author Joacim Zschimmer
 */
public class CommandDispatcherTest {
    private CommandDispatcher commandDispatcher = commandDispatcher();


    private static CommandDispatcher commandDispatcher() {
        CommandExecutor[] e1 = { Executor1.singleton };
        CommandExecutor[] e2 = { Executor2.singleton };
        XmlCommandParser[] p1 = { Parser1.singleton };
        XmlCommandParser[] p2 = { Parser2.singleton };
        ResultXmlizer[] x1 = { Xmlizer1.singleton };
        ResultXmlizer[] x2 = { Xmlizer2.singleton };
        CommandSuite[] suites = { new CommandSuite(Arrays.asList(e1), Arrays.asList(p1), Arrays.asList(x1)),
                                  new CommandSuite(Arrays.asList(e2), Arrays.asList(p2), Arrays.asList(x2)) };
        CommandSuite completeSuite = CommandSuite.of(Arrays.asList(suites));
        return new CommandDispatcher(Collections.singleton(completeSuite));
    }


    @Test public void testExecuteXml_1() {
        String resultXml = commandDispatcher.executeXml("<command1 value1='VALUE 1'/>");
        Element e = loadXml(resultXml).getDocumentElement();
        assertThat(e.getNodeName(), equalTo("result1"));
        assertThat(e.getAttribute("value1"), equalTo("VALUE 1"));
    }


    @Test public void testExecuteXml_2() {
        String resultXml = commandDispatcher.executeXml("<command2 value2='VALUE 2'/>");
        Element e = loadXml(resultXml).getDocumentElement();
        assertThat(e.getNodeName(), equalTo("result2"));
        assertThat(e.getAttribute("value2"), equalTo("VALUE 2"));
    }
}
