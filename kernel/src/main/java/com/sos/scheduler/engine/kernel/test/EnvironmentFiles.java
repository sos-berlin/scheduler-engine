package com.sos.scheduler.engine.kernel.test;

import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.transform;
import static com.sos.scheduler.engine.kernel.util.Files.copyURLToFile;
import static java.util.Arrays.asList;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import com.google.common.base.Function;
import com.google.common.collect.ImmutableList;

public class EnvironmentFiles {
    private final Package testPackage;
    private final File directory;
    private final ImmutableList<URL> resourceUrls;
    private final PathMatchingResourcePatternResolver resolver = new PathMatchingResourcePatternResolver();

    public EnvironmentFiles(Package testPackage, File directory) {
        this.testPackage = testPackage;
        this.directory = directory;
        resourceUrls = resourceUrls();
    }

    private ImmutableList<URL> resourceUrls() {
        Map<String,URL> result = new LinkedHashMap<String,URL>();
        for (URL u: resourceUrls(getClass().getPackage())) result.put(nameOfUrl(u), u);
        for (URL u: resourceUrls(testPackage)) result.put(nameOfUrl(u), u);
        return ImmutableList.copyOf(result.values());
    }

    private Iterable<URL> resourceUrls(Package p) {
        return transform(resources(p), getUrl);
    }

    private Iterable<Resource> resources(Package p) {
        return concat(resources(p, "*.xml"), resources(p, "*.ini"));
    }

    private List<Resource> resources(Package p, String namePattern) {
        try {
            return asList(resolver.getResources("classpath*:" + directoryOfPackage(p) + "/" + namePattern));
        } catch (IOException x) { throw new RuntimeException(x); }
    }

    private void copy() {
        for (URL u: resourceUrls) copyResource(u);
    }

    private void copyResource(URL url) {
        copyURLToFile(url, new File(directory, nameOfUrl(url)));
    }

    private static String nameOfUrl(URL u) {
        return new File(u.getPath()).getName();
    }

    static String directoryOfClass(Class<?> c) {
        return directoryOfPackage(c.getPackage());
    }

    private static String directoryOfPackage(Package p) {
        return p.getName().replace('.', '/');
    }

    static void copy(Package testPackage, File directory) {
        new EnvironmentFiles(testPackage, directory).copy();
    }

    private static final Function<Resource,URL> getUrl = new Function<Resource,URL>() {
        @Override public URL apply(Resource o) {
            try {
                return o.getURL();
            } catch (IOException e) { throw new RuntimeException(e); }
        }
    };
}
