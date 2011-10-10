package com.sos.scheduler.engine.kernel.util;

import static com.sos.scheduler.engine.kernel.util.Files.*;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.FileOutputStream;

import org.junit.After;
import org.junit.Test;

public class FilesTest {
    private final File dir = makeTemporaryDirectory();

    @After public void after() {
        Files.removeDirectoryRecursivlyFollowingLinks(dir);
    }

    @Test public void testDeleteFile() throws Exception {
        File a =new File(dir, "a");
        new FileOutputStream(a).close();
        assertTrue(a.exists());
        Files.removeFile(a);
        assertFalse(a.exists());
    }

    @Test public void testMakeDirectory() throws Exception {
        File a = new File(dir, "a");
        makeDirectory(a);
        assertTrue(a.isDirectory());
    }

    @Test public void testMakeTemporaryDirectory() throws Exception {
        File a = makeTemporaryDirectory();
        assertTrue(a.isDirectory());
    }

    @Test public void testRemoveDirectoryRecursivlyFollowingLinks() throws Exception {
        File a = new File(dir, "a");
        assertTrue(a.mkdir());
        File b = new File(a, "b");
        assertTrue(b.mkdir());
        assertTrue(a.isDirectory());
        new FileOutputStream(new File(b, "c")).close();
        removeDirectoryRecursivlyFollowingLinks(a);
        assertFalse(a.exists());
        assertTrue(dir.exists());
    }
}
