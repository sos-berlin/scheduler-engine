package com.sos.scheduler.engine.kernel.util;

import static com.google.common.base.Preconditions.checkArgument;
import static com.google.common.io.Files.createTempDir;
import static org.apache.commons.io.IOUtils.closeQuietly;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;

import org.apache.commons.io.IOUtils;

public final class Files {
    private Files() {}

    public static void removeFile(File f) {
        boolean ok = f.delete();
        if (!ok  &&  f.exists())  throw new RuntimeException("File cannot be deleted: " + f);
    }

    public static void makeDirectory(File dir) {
        boolean ok = dir.mkdir();
        if (!ok  &&  !dir.isDirectory())  throw new RuntimeException("Directory cannot be created: " + dir);
    }

    public static File makeTemporaryDirectory() {
        try {
            return createTempDir().getCanonicalFile();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public static void removeDirectoryRecursivlyFollowingLinks(File directory) {
        checkArgument(directory.isDirectory(), "Not a directory: %s", directory);
        // Prüfung, ob Datei ein symbolischer Link ist, in C++ implementieren? Abhängigkeiten der Artefakte beachten.
        File[] files = directory.listFiles();
        if (files == null)
            throw new RuntimeException("Error listing files for " + directory);
        for (File file: files) {
            if (file.isDirectory()) removeDirectoryRecursivlyFollowingLinks(file);
            else removeFile(file);
        }
        removeFile(directory);
    }

    public static void copyURLToFile(URL source, File destination) {
        try {
            InputStream in = source.openStream();
            OutputStream out = new FileOutputStream(destination);
            try {
                IOUtils.copy(in, out);
                out.close();
            } finally {
                closeQuietly(out);
                closeQuietly(in);
            }
        } catch (IOException x) {
            throw new RuntimeException(x);
        }
    }
}
