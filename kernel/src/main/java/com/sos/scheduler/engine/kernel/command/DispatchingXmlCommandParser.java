package com.sos.scheduler.engine.kernel.command;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.SchedulerException;
import java.util.Arrays;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


public class DispatchingXmlCommandParser implements CommandXmlParser {
    private final ImmutableMap<String,SingleCommandXmlParser> singleCommandParsers;
    private final ImmutableMap<String,PrefixCommandXmlParser> prefixCommandParsers;


    public DispatchingXmlCommandParser(Iterable<CommandXmlParser> parsers) {
        ImmutableMap.Builder<String,SingleCommandXmlParser> builder = new ImmutableMap.Builder<String,SingleCommandXmlParser>();
        ImmutableMap.Builder<String,PrefixCommandXmlParser> prefixBuilder = new ImmutableMap.Builder<String,PrefixCommandXmlParser>();
        for (CommandXmlParser p: parsers) {
            if (p instanceof PrefixCommandXmlParser) {
                PrefixCommandXmlParser pp = (PrefixCommandXmlParser)p;
                prefixBuilder.put(pp.getCommandPrefix(), pp);
            } else
            if (p instanceof SingleCommandXmlParser) {
                SingleCommandXmlParser pp = (SingleCommandXmlParser)p;
                builder.put(pp.getCommandName(), pp);
            } else
                throw new SchedulerException("Unknown CommandParser " + p);
        }
        singleCommandParsers = builder.build();
        prefixCommandParsers = prefixBuilder.build();
    }


    public DispatchingXmlCommandParser(CommandXmlParser... parsers) {
        this(Arrays.asList(parsers));
    }

    
    @Override public final Command parse(Element element) {
        return commandParser(element).parse(element);
    }


    private CommandXmlParser commandParser(Element e) {
        CommandXmlParser result = null;
        String name = e.getNodeName();
        int t = name.indexOf(PrefixCommandXmlParser.prefixTerminator);
        if (t >= 0)  result = prefixCommandParsers.get(name.substring(0, t + 1));
        if (result == null)  result = singleCommandParsers.get(name);
        if (result == null)  throw new UnknownCommandException("<" + e.getNodeName() + ">");
        return result;
    }
}
