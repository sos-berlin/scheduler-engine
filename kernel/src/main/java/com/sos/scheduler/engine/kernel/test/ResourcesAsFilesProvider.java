package com.sos.scheduler.engine.kernel.test;

import static com.sos.scheduler.engine.kernel.util.Classes.springPattern;
import static com.sos.scheduler.engine.kernel.util.Files.copyURLToFile;

import java.io.File;
import java.io.IOException;

import org.apache.log4j.Logger;
import org.springframework.core.io.FileSystemResource;
import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

/** Stellt Resourcen als Dateien zur Verfügung -
 * basiert auf {@link PathMatchingResourcePatternResolver}, das nicht in jeder Umgebung funktionieren muss.
 * Ist also nur für den Test des Schedulers geeignet, nicht unbedingt für die Produktion. */
class ResourcesAsFilesProvider {
    private static final Logger logger = Logger.getLogger(ResourcesAsFilesProvider.class);
    private static final PathMatchingResourcePatternResolver resourceResolver = new PathMatchingResourcePatternResolver();

    private final File directory;
    private final ImmutableList<Resource> resources;

    ResourcesAsFilesProvider(String resourcePattern, File directory) {
        logger.info("******************");
        try {
            this.directory = directory;
            resources = ImmutableList.copyOf(resourceResolver.getResources(resourcePattern));
            if (resources.isEmpty())
                logger.warn("No resources in " + resourcePattern);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private ImmutableMap<String,ResourceFile> apply() {
        ImmutableMap.Builder<String,ResourceFile> result = new ImmutableMap.Builder<String,ResourceFile>();
        for (Resource r: resources) {
            ResourceFile f = provideResourceAsFile(r);
            result.put(r.getFilename(), f);
        }
        return result.build();
    }

    private ResourceFile provideResourceAsFile(Resource r) {
        logger.debug(r+": "+r.getClass().getName());
        if (r instanceof FileSystemResource)
            return new ResourceFile(((FileSystemResource)r).getFile(), false);
        else
            return provideNonfileResourceAsFile(r);
    }

    private ResourceFile provideNonfileResourceAsFile(Resource r) {
        try {
            logger.debug("provideNonfileResourceAsFile("+r+")");
            File f = new File(directory, r.getFilename());
            if (!resourceSeemsEqualToFile(r, f)) {
                copyResource(r, f);
                return new ResourceFile(f, true);
            } else
                return new ResourceFile(f, false);
        } catch (IOException x) { throw new RuntimeException("Error while providing "+r+": "+x, x); }
    }

    private static void copyResource(Resource r, File f) throws IOException {
        logger.debug("copyURLToFile(" + r.getURL() + ", " + f + ")");
        copyURLToFile(r.getURL(), f);
        f.setLastModified(r.lastModified());
        logger.debug(r + " extracted to " + f);
        assert resourceSeemsEqualToFile(r, f);
    }

    private static boolean resourceSeemsEqualToFile(Resource r, File f) throws IOException {
        return f.exists()  &&  f.lastModified() == r.lastModified()  &&  f.length() == r.contentLength();
    }

    static ImmutableMap<String,ResourceFile> provideResourcesAsFiles(Package pack, File directory) {
        return provideResourcesAsFiles(springPattern(pack), directory);
    }

    static ImmutableMap<String,ResourceFile> provideResourcesAsFiles(String resourcePattern, File directory) {
        return new ResourcesAsFilesProvider(resourcePattern, directory).apply();
    }
}
