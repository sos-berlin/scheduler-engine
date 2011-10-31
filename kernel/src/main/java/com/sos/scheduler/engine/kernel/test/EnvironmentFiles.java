package com.sos.scheduler.engine.kernel.test;

import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.transform;
import static com.sos.scheduler.engine.kernel.util.Files.copyURLToFile;
import static java.util.Arrays.asList;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.Date;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import com.google.common.base.Function;
import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.util.ResourcePath;

public class EnvironmentFiles {
    private static final ResourcePath defaultConfigResourcePath = new ResourcePath(EnvironmentFiles.class.getPackage(), "config");
    private final ResourcePath configResourcePath;
    private final File directory;
    private final ImmutableList<URL> resourceUrls;
    private final PathMatchingResourcePatternResolver resolver = new PathMatchingResourcePatternResolver();
    private final long lastModified = new Date().getTime() - 3000;  // 3s zurück, um bei 1s (oder bei FAT 2s) Zeitraster in der Vergangenheit zu liegen

    public EnvironmentFiles(ResourcePath configResourcePath, File directory) {
        this.configResourcePath = configResourcePath;
        this.directory = directory;
        resourceUrls = resourceUrls();
    }

    private ImmutableList<URL> resourceUrls() {
        Map<String,URL> result = new LinkedHashMap<String,URL>();
        for (URL u: resourceUrls(defaultConfigResourcePath)) result.put(nameOfUrl(u), u);
        for (URL u: resourceUrls(configResourcePath)) result.put(nameOfUrl(u), u);
        return ImmutableList.copyOf(result.values());
    }

    private Iterable<URL> resourceUrls(ResourcePath p) {
        return transform(resources(p), getUrl);
    }

    private Iterable<Resource> resources(ResourcePath p) {
        return concat(resources(p, "*.xml"), resources(p, "*.ini"));
    }

    private List<Resource> resources(ResourcePath p, String namePattern) {
        try {
            return asList(resolver.getResources("classpath*:"+ p.path() +"/"+namePattern));
        } catch (IOException x) { throw new RuntimeException(x); }
    }

    private void copy() {
        for (URL u: resourceUrls) copyResource(u);
    }

    private void copyResource(URL url) {
        File f = new File(directory, nameOfUrl(url));
        copyURLToFile(url, f);
        f.setLastModified(lastModified);
    }


    static void copy(ResourcePath configResourcePath, File directory) {
        new EnvironmentFiles(configResourcePath, directory).copy();
    }

    private static final Function<Resource,URL> getUrl = new Function<Resource,URL>() {
        @Override public URL apply(Resource o) {
            try {
                return o.getURL();
            } catch (IOException e) { throw new RuntimeException(e); }
        }
    };

    private static String nameOfUrl(URL u) {
        return new File(u.getPath()).getName();
    }
}
