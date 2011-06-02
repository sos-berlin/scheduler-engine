package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


public class CommandDispatcher {
    private final DispatchingCommandExecutor executor;
    private final DispatchingXmlCommandParser parser;
    private final DispatchingResultXmlizer resultXmlizer;


    public CommandDispatcher(Iterable<CommandSuite> suites) {
        CommandSuite combinedSuite = CommandSuite.of(suites);
        executor = new DispatchingCommandExecutor(combinedSuite.getExecutors());
        parser = new DispatchingXmlCommandParser(combinedSuite.getParsers());
        resultXmlizer = new DispatchingResultXmlizer(combinedSuite.getResultXmlizer());
    }


    public String executeXml(String xml) {
        Command command = parse(loadXml(xml).getDocumentElement());
        Result result = executeCommand(command);
        return toXml(elementOfResult(result));
    }


    private Result executeCommand(Command c) {
        return executor.execute(c);
    }


    private Command parse(Element e) {
        return parser.parse(e);
    }


    private Element elementOfResult(Result r) {
        return resultXmlizer.toElement(r);
    }
}
