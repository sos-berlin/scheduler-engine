package com.sos.scheduler.engine.kernelcpptest.xmlcommand.job_why;

import com.google.common.base.Function;
import java.util.Collection;
import java.util.List;
import static com.google.common.collect.Collections2.transform;
import static com.google.common.collect.Iterables.concat;
import static java.util.Arrays.asList;


final class Configuration {
    static final List<String> jobNames = asList("a", "b", "c", "minTasks", "maxTasks");
    static final Collection<String> jobFilenames = transform(jobNames, append(".job.xml"));
    static final String jobchainName = "j";
    static final Iterable<String> configFilenames = concat(jobFilenames, 
            asList(jobchainName + ".job_chain.xml", "j,never.order.xml"));


    private Configuration() {}

    
    private static Function<String,String> append(final String appendix) {
        return new Function<String,String>() {
            @Override public String apply(String a) {
                return a + appendix;
            }
        };
    }
}
