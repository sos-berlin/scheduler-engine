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
import org.apache.log4j.Logger;

public final class Files {
    private static final Logger logger = Logger.getLogger(Files.class);

    private Files() {}

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

    public static void removeDirectoryRecursivly(File directory) {
        try {
            checkArgument(directory.isDirectory(), "Not a directory: %s", directory);
            removeAbsoluteDirectoryRecursivly(directory.getCanonicalFile().getAbsoluteFile());
        } catch (IOException x) { throw new RuntimeException(x); }
    }

    private static void removeAbsoluteDirectoryRecursivly(File dir) throws IOException {
        String[] names = dir.list();
        if (names == null)
            throw new RuntimeException("Error listing files for "+dir);
        if (names.length > 0) {
            if (directoryCouldBeALink(dir, names[0]))
                logger.debug("Seems to be a link and will not be deleted: "+dir);
            else {
                for (String name: names) {
                    File f = new File(dir, name);
                    if (f.isDirectory()) removeAbsoluteDirectoryRecursivly(f);
                    else removeFile(f);
                }
            }
        }
        removeFile(dir);
    }

    private static boolean directoryCouldBeALink(File dir, String someDirectoryEntry) throws IOException {
        return !new File(dir, someDirectoryEntry).getCanonicalFile().getParentFile().equals(dir);
    }

    public static void removeFile(File f) {
        boolean ok = f.delete();
        if (!ok  &&  f.exists())  throw new RuntimeException("File cannot be deleted: " + f);
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
