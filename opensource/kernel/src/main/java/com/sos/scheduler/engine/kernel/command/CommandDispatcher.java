package com.sos.scheduler.engine.kernel.command;

import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import java.util.ArrayList;
import java.util.List;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


public class CommandDispatcher {
    private final DispatchingCommandExecutor executor;
    private final DispatchingXmlCommandParser parser;
    private final DispatchingResultXmlizer resultXmlizer;


//    public CommandDispatcher(Iterable<CommandSuite> suites) {
//        CommandSuite combinedSuite = CommandSuite.of(suites);
//        executor = new DispatchingCommandExecutor(combinedSuite.getExecutors());
//        parser = new DispatchingXmlCommandParser(combinedSuite.getParsers());
//        resultXmlizer = new DispatchingResultXmlizer(combinedSuite.getResultXmlizer());
//    }


    public CommandDispatcher(Iterable<CommandHandler> handlers) {
        List<CommandExecutor> executors = new ArrayList<CommandExecutor>();
        List<CommandXmlParser> parsers = new ArrayList<CommandXmlParser>();
        List<ResultXmlizer> xmlizers = new ArrayList<ResultXmlizer>();
        
        for (CommandHandler handler: handlers) {
            if (handler instanceof CommandExecutor)
                executors.add((CommandExecutor)handler);
            else
            if (handler instanceof CommandXmlParser)
                parsers.add((CommandXmlParser)handler);
            else
            if (handler instanceof ResultXmlizer)
                xmlizers.add((ResultXmlizer)handler);
            else
                throw new SchedulerException("CommandDispatcher " + handler);
        }
        executor = new DispatchingCommandExecutor(executors);
        parser = new DispatchingXmlCommandParser(parsers);
        resultXmlizer = new DispatchingResultXmlizer(xmlizers);
    }


    public final String executeXml(String xml) {
        Command command = parse(loadXml(xml).getDocumentElement());
        Result result = execute(command);
        return toXml(elementOfResult(result));
    }


    public final Result execute(Command c) {
        return executor.execute(c);
    }


    public final Command parse(Element e) {
        return parser.parse(e);
    }


    public final Element elementOfResult(Result r) {
        return resultXmlizer.toElement(r);
    }
}
