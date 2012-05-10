package com.sos.scheduler.engine.test;

import com.google.common.base.Function;
import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.util.ResourcePath;
import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import javax.annotation.Nullable;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.Date;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import static com.google.common.base.Objects.firstNonNull;
import static com.google.common.collect.Iterables.concat;
import static com.google.common.collect.Iterables.transform;
import static java.util.Arrays.asList;

public class EnvironmentFiles {
    private static final ResourcePath defaultConfigResourcePath = new ResourcePath(EnvironmentFiles.class.getPackage(), "config");
    private final ResourcePath configResourcePath;
    private final File directory;
    private final ImmutableMap<String,URL> resourceUrls;
    private final PathMatchingResourcePatternResolver resolver = new PathMatchingResourcePatternResolver();
    private final long lastModified = new Date().getTime() - 3000;  // 3s zurück, um bei 1s (oder bei FAT 2s) Zeitraster in der Vergangenheit zu liegen
    private final ImmutableMap<String,String> nameMap;
    private final ResourceToFileTransformer fileTransformer;

    public EnvironmentFiles(ResourcePath configResourcePath, File directory,
            @Nullable ImmutableMap<String,String> nameMap,
            @Nullable ResourceToFileTransformer fileTransformer) {
        this.configResourcePath = configResourcePath;
        this.directory = directory;
        this.nameMap = firstNonNull(nameMap, ImmutableMap.<String, String>of());
        this.fileTransformer = firstNonNull(fileTransformer, StandardResourceToFileTransformer.singleton);
        resourceUrls = resourceUrls();
    }

    private ImmutableMap<String,URL> resourceUrls() {
        Map<String,URL> result = new LinkedHashMap<String,URL>();
        for (URL u: resourceUrls(defaultConfigResourcePath)) result.put(nameOfUrl(u), u);
        for (URL u: resourceUrls(configResourcePath)) result.put(nameOfUrl(u), u);
        return ImmutableMap.copyOf(result);
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
        for (Map.Entry<String,URL> i: resourceUrls.entrySet()) copyResource(i.getValue(), i.getKey());
    }

    private void copyResource(URL url, String name) {
        File f = new File(directory, name);
        fileTransformer.transform(url, f);
        f.setLastModified(lastModified);
    }

    static void copy(ResourcePath configResourcePath, File directory) {
        new EnvironmentFiles(configResourcePath, directory, null, null).copy();
    }

    static void copy(ResourcePath configResourcePath, File directory,
                     @Nullable ImmutableMap<String,String> nameMap, @Nullable ResourceToFileTransformer fileTransformer) {
        new EnvironmentFiles(configResourcePath, directory, nameMap, fileTransformer).copy();
    }

    private static final Function<Resource,URL> getUrl = new Function<Resource,URL>() {
        @Override public URL apply(Resource o) {
            try {
                return o.getURL();
            } catch (IOException e) { throw new RuntimeException(e); }
        }
    };

    private String nameOfUrl(URL u) {
        String name = new File(u.getPath()).getName();
        String newName = nameMap.get(name);
        return firstNonNull(newName, name);
    }
}
