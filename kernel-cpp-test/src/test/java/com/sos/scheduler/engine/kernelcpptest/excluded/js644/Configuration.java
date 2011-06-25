package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import com.google.common.base.Function;
import java.util.Collection;
import java.util.List;
import static com.google.common.collect.Collections2.transform;
import static com.google.common.collect.Iterables.concat;
import static java.util.Arrays.asList;


class Configuration {
    static final List<String> jobPaths = asList("exercise10a", "exercise10b", "exercise10c");
    static final Collection<String> jobFilenames = transform(jobPaths, append(".job.xml"));
    static final String jobchainName = "exercise10";
    static final Iterable<String> configFilenames = concat(jobFilenames, asList(jobchainName + ".job_chain.xml", "exercise10,1.order.xml"));


    private static Function<String,String> append(final String appendix) {
        return new Function<String,String>() {
            @Override public String apply(String a) {
                return a + appendix;
            }
        };
    }
}
