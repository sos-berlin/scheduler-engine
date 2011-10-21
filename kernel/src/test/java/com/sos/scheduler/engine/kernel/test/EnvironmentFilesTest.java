package com.sos.scheduler.engine.kernel.test;

import static com.google.common.io.Files.createTempDir;
import static com.sos.scheduler.engine.kernel.util.Classes.springPattern;
import static com.sos.scheduler.engine.kernel.util.Files.removeFile;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.arrayContainingInAnyOrder;
import static org.hamcrest.Matchers.arrayWithSize;
import static org.hamcrest.Matchers.emptyArray;

import java.io.File;
import java.io.IOException;

import org.junit.Test;
import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import com.sos.scheduler.engine.kernel.util.ResourcePath;

public final class EnvironmentFilesTest {
    private static final String[] expectedNames = {"scheduler.xml", "factory.ini", "sos.ini"};

    @Test public void test() {
        File dir = createTempDir();
        try {
            assertThat(dir.list(), emptyArray());
            EnvironmentFiles.copy(new ResourcePath(EnvironmentFilesTest.class.getPackage(), "config"), dir);
            assertThat(dir.list(), arrayContainingInAnyOrder(expectedNames));
        } finally {
            for (String name: expectedNames) removeFile(new File(dir, name));
            removeFile(dir);
        }
    }

    @Test public void testSpringGetResources() throws IOException {
        PathMatchingResourcePatternResolver r = new PathMatchingResourcePatternResolver();
        Resource[] result = r.getResources(springPattern(EnvironmentFilesTest.class.getPackage(), "config/scheduler.xml"));
        assertThat(result, arrayWithSize(1));
    }
}
