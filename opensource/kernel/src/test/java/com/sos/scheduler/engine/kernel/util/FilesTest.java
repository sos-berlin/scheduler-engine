package com.sos.scheduler.engine.kernel.util;

import org.junit.After;
import org.junit.Test;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import static com.sos.scheduler.engine.kernel.util.Files.*;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public final class FilesTest {
    private final File dir = makeTemporaryDirectory();

    @After public void after() {
        removeDirectoryRecursivly(dir);
    }

    @Test public void testMakeTemporaryDirectory() throws IOException {
        assertTrue(dir.isDirectory());
    }

    @Test public void testDeleteFile() throws IOException {
        File a = new File(dir, "a");
        new FileOutputStream(a).close();
        assertTrue(a.exists());
        removeFile(a);
        assertFalse(a.exists());
    }

    @Test public void testMakeDirectory() throws IOException {
        File a = new File(dir, "a");
        makeDirectory(a);
        assertTrue(a.isDirectory());
    }

    @Test public void testRemoveDirectoryRecursivlyFollowingLinks() throws IOException {
        File a = new File(dir, "a");
        assertTrue(a.mkdir());
        File b = new File(a, "b");
        assertTrue(b.mkdir());
        assertTrue(a.isDirectory());
        new FileOutputStream(new File(b, "c")).close();
        removeDirectoryRecursivly(a);
        assertFalse(a.exists());
        assertTrue(dir.exists());
    }
}
