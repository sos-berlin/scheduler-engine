package com.sos.scheduler.engine.kernel.command;

import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;


public class CommandSuite {
    private final ImmutableCollection<CommandExecutor> executors;
    private final ImmutableCollection<CommandXmlParser> parsers;
    private final ImmutableCollection<ResultXmlizer> resultXmlizer;


    public CommandSuite(Iterable<CommandExecutor> executors, Iterable<CommandXmlParser> parsers,
            Iterable<ResultXmlizer> resultXmlizers) {
        this.executors =  ImmutableList.copyOf(executors);
        this.parsers = ImmutableList.copyOf(parsers);
        this.resultXmlizer = ImmutableList.copyOf(resultXmlizers);
    }


    public CommandSuite(CommandExecutor[] executors, CommandXmlParser[] parsers, ResultXmlizer[] resultXmlizers) {
        this(Arrays.asList(executors), Arrays.asList(parsers), Arrays.asList(resultXmlizers));
    }


    public ImmutableCollection<CommandExecutor> getExecutors() {
        return executors;
    }

    public ImmutableCollection<CommandXmlParser> getParsers() {
        return parsers;
    }

    public ImmutableCollection<ResultXmlizer> getResultXmlizer() {
        return resultXmlizer;
    }


    public static CommandSuite of(Iterable<CommandSuite> suites ) {
        List<CommandExecutor> executors = new ArrayList<CommandExecutor>();
        List<CommandXmlParser> parsers = new ArrayList<CommandXmlParser>();
        List<ResultXmlizer> xmlizers = new ArrayList<ResultXmlizer>();
        for (CommandSuite s: suites) {
            executors.addAll(s.getExecutors());
            parsers.addAll(s.getParsers());
            xmlizers.addAll(s.getResultXmlizer());
        }
        return new CommandSuite(executors, parsers, xmlizers);
    }
}
