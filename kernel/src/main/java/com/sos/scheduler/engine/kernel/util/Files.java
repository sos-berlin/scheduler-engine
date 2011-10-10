package com.sos.scheduler.engine.kernel.util;

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

    public static void deleteFile(File f) {
        boolean ok = f.delete();
        if (!ok  &&  f.exists())  throw new RuntimeException("File cannot be deleted: " + f);
    }

    public static void copyURLToFile(URL source, File destination) throws IOException {
        InputStream in = source.openStream();
        try {
            OutputStream out = new FileOutputStream(destination);
            try {
                IOUtils.copy(in, out);
                out.close();
            } finally {
                closeQuietly(out);
            }
        } finally {
            closeQuietly(in);
        }
    }
}
