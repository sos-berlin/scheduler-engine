package com.sos.scheduler.engine.kernel.test.binary;

import static com.sos.scheduler.engine.kernel.test.binary.ResourcesAsFilesProvider.provideResourcesAsFiles;
import static com.sos.scheduler.engine.kernel.util.Classes.springPattern;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsInAnyOrder;

import java.io.IOException;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.springframework.core.io.Resource;
import org.springframework.core.io.support.PathMatchingResourcePatternResolver;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;

public class ResourcesAsFilesProviderTest {
    private static final String[] expectedFilenames = {"scheduler.xml", "factory.ini", "sos.ini"};
    private static final PathMatchingResourcePatternResolver resourceResolver = new PathMatchingResourcePatternResolver();

    @Rule public final TemporaryFolder folder = new TemporaryFolder();

    @Test public void test() throws IOException {
        String pattern = springPattern(SchedulerTest.class.getPackage(), "config/*");
        ImmutableList<Resource> resources = ImmutableList.copyOf(resourceResolver.getResources(pattern));
        ImmutableMap<String,ResourceFile> map = provideResourcesAsFiles(resources, folder.getRoot());
        assertThat(map.keySet(), containsInAnyOrder(expectedFilenames));
//        String[] list = dir.list();
//        assertThat(list, arrayContainingInAnyOrder(expectedFilenames));
    }
}
