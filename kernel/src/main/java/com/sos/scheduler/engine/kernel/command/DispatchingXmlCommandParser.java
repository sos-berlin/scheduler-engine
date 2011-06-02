package com.sos.scheduler.engine.kernel.command;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.SchedulerException;
import java.util.Arrays;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


public class DispatchingXmlCommandParser implements XmlCommandParser {
    private final ImmutableMap<String,SingleXmlCommandParser> singleCommandParsers;
    private final ImmutableMap<String,PrefixXmlCommandParser> prefixCommandParsers;


    public DispatchingXmlCommandParser(Iterable<XmlCommandParser> parsers) {
        ImmutableMap.Builder<String,SingleXmlCommandParser> builder = new ImmutableMap.Builder<String,SingleXmlCommandParser>();
        ImmutableMap.Builder<String,PrefixXmlCommandParser> prefixBuilder = new ImmutableMap.Builder<String,PrefixXmlCommandParser>();
        for (XmlCommandParser p: parsers) {
            if (p instanceof PrefixXmlCommandParser) {
                PrefixXmlCommandParser pp = (PrefixXmlCommandParser)p;
                prefixBuilder.put(pp.getCommandPrefix(), pp);
            } else
            if (p instanceof SingleXmlCommandParser) {
                SingleXmlCommandParser pp = (SingleXmlCommandParser)p;
                builder.put(pp.getCommandName(), pp);
            } else
                throw new SchedulerException("Unknown CommandParser " + p);
        }
        singleCommandParsers = builder.build();
        prefixCommandParsers = prefixBuilder.build();
    }


    public DispatchingXmlCommandParser(XmlCommandParser... parsers) {
        this(Arrays.asList(parsers));
    }

    
    @Override public final Command parse(Element element) {
        return commandParser(element).parse(element);
    }


    private XmlCommandParser commandParser(Element e) {
        XmlCommandParser result = null;
        String name = e.getNodeName();
        int t = name.indexOf(PrefixXmlCommandParser.prefixTerminator);
        if (t >= 0)  result = prefixCommandParsers.get(name.substring(0, t + 1));
        if (result == null)  result = singleCommandParsers.get(name);
        if (result == null)  throw new UnknownCommandException("<" + e.getNodeName() + ">");
        return result;
    }
}
