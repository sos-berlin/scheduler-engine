package com.sos.scheduler.engine.kernel.command;

import org.junit.Test;

import java.util.Arrays;

import static com.sos.scheduler.engine.common.xml.CppXmlUtils.loadXml;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

/**
 *
 * @author Joacim Zschimmer
 */
public class DispatchingCommandParserTest {
    private final DispatchingXmlCommandParser parser = dispatchingCommandParser();

    private static DispatchingXmlCommandParser dispatchingCommandParser() {
        CommandXmlParser[] parsers = {ACommandXmlParser.singleton, BCommandXmlParser.singleton};
        return new DispatchingXmlCommandParser(Arrays.asList(parsers));
    }

    @Test public void testParse_1() {
        Command result = parser.parse(loadXml("<a value1='VALUE 1'/>").getDocumentElement());
        ACommand c = (ACommand)result;
        assertThat(c.value, equalTo("VALUE 1"));
    }

    @Test public void testExecuteXml_2() {
        Command result = parser.parse(loadXml("<b value2='VALUE 2'/>").getDocumentElement());
        BCommand c = (BCommand)result;
        assertThat(c.value, equalTo("VALUE 2"));
    }
}

