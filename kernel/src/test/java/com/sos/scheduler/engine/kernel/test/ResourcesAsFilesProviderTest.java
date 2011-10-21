package com.sos.scheduler.engine.kernel.test;

import static com.sos.scheduler.engine.kernel.util.Classes.springPattern;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsInAnyOrder;

import java.io.File;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

import com.google.common.collect.ImmutableMap;

public class ResourcesAsFilesProviderTest {
    private static final String[] expectedFilenames = {"scheduler.xml", "factory.ini", "sos.ini"};

    @Rule public final TemporaryFolder folder = new TemporaryFolder();

    @Test public void test() {
        File dir = folder.getRoot();
        ImmutableMap<String,ResourceFile> map = ResourcesAsFilesProvider.provideResourcesAsFiles(springPattern(ResourcesAsFilesProviderTest.class.getPackage(), "config/*"), dir);
        assertThat(map.keySet(), containsInAnyOrder(expectedFilenames));
//        String[] list = dir.list();
//        assertThat(list, arrayContainingInAnyOrder(expectedFilenames));
    }
}
