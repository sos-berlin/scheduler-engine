package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import com.google.common.base.Function;
import java.util.Collection;
import java.util.List;
import static com.google.common.collect.Collections2.transform;
import static com.google.common.collect.Iterables.concat;
import static java.util.Arrays.asList;

final class Configuration {
    private Configuration() {}

    static final List<String> jobPaths = asList("exercise10a", "exercise10b", "exercise10c");
    static final Collection<String> jobFilenames = transform(jobPaths, append(".job.xml"));

    private static Function<String,String> append(final String appendix) {
        return new Function<String,String>() {
            @Override public String apply(String a) {
                return a + appendix;
            }
        };
    }
}
