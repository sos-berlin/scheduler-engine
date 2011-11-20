package com.sos.scheduler.engine.kernel.command;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import org.w3c.dom.Element;
import static java.util.Arrays.asList;


class DispatchingXmlCommandParser implements CommandXmlParser {
    private final ImmutableMap<String,SingleCommandXmlParser> singleCommandParsers;
    private final ImmutableMap<String,PrefixCommandXmlParser> prefixCommandParsers;


    DispatchingXmlCommandParser(Iterable<CommandXmlParser> parsers) {
        ImmutableMap.Builder<String,SingleCommandXmlParser> builder = new ImmutableMap.Builder<String,SingleCommandXmlParser>();
        ImmutableMap.Builder<String,PrefixCommandXmlParser> prefixBuilder = new ImmutableMap.Builder<String,PrefixCommandXmlParser>();
        for (CommandXmlParser p: parsers) {
            if (p instanceof SingleCommandXmlParser) {
                SingleCommandXmlParser pp = (SingleCommandXmlParser)p;
                builder.put(pp.getCommandName(), pp);
            } else
            if (p instanceof PrefixCommandXmlParser) {
                PrefixCommandXmlParser pp = (PrefixCommandXmlParser)p;
                prefixBuilder.put(pp.getCommandPrefix(), pp);
            } else
                throw new SchedulerException("Unknown CommandParser " + p);
        }
        singleCommandParsers = builder.build();
        prefixCommandParsers = prefixBuilder.build();
    }


    DispatchingXmlCommandParser(CommandXmlParser... parsers) {
        this(asList(parsers));
    }

    
    @Override public final Command parse(Element element) {
        return commandParser(element.getNodeName()).parse(element);
    }


    private CommandXmlParser commandParser(String name) {
        CommandXmlParser result = null;
        int t = name.indexOf(PrefixCommandXmlParser.prefixTerminator);
        if (t >= 0)  result = prefixCommandParsers.get(name.substring(0, t + 1));
        if (result == null)  result = singleCommandParsers.get(name);
        if (result == null)  throw new UnknownCommandException("<" + name + ">");
        return result;
    }
}
