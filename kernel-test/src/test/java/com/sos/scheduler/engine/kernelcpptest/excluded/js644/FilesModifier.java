package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import static com.google.common.collect.Iterators.emptyIterator;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Iterator;

import org.apache.log4j.Logger;

import com.google.common.base.Charsets;
import com.google.common.collect.ImmutableList;
import com.google.common.io.Files;

public final class FilesModifier {
    private static final Charset encoding = Charsets.UTF_8;
    private static final Logger logger = Logger.getLogger(FilesModifier.class);

    private final ImmutableList<File> files;
    private Iterator<File> iterator = emptyIterator();
    private int number = 0;

    public FilesModifier(Iterable<File> files) {
        this.files = ImmutableList.copyOf(files);
    }

    public void modifyNext() {
        if (!iterator.hasNext())
            iterator = files.iterator();
        modifyFile(iterator.next());
    }

    private void modifyFile(File f) {
        try {
            logger.debug("modifyFile #"+number+" "+f);
            number++;
            assert f.exists() : "Datei fehlt: " + f;
            Files.append(" ", f, encoding);
        } catch (IOException x) { throw new RuntimeException(x); }
    }

    public int fileCount() {
        return files.size();
    }
}
