package com.sos.scheduler.engine.kernel.command;

import java.util.Arrays;
import org.junit.Test;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

/**
 *
 * @author Joacim Zschimmer
 */
public class DispatchingCommandParserTest {
    private DispatchingXmlCommandParser parser = dispatchingCommandParser();


    private static DispatchingXmlCommandParser dispatchingCommandParser() {
        XmlCommandParser[] parsers = {Parser1.singleton, Parser2.singleton};
        return new DispatchingXmlCommandParser(Arrays.asList(parsers));
    }


    @Test public void testParse_1() {
        Command result = parser.parse(loadXml("<command1 value1='VALUE 1'/>").getDocumentElement());
        Command1 c = (Command1)result;
        assertThat(c.value, equalTo("VALUE 1"));
    }


    @Test public void testExecuteXml_2() {
        Command result = parser.parse(loadXml("<command2 value2='VALUE 2'/>").getDocumentElement());
        Command2 c = (Command2)result;
        assertThat(c.value, equalTo("VALUE 2"));
    }
}

