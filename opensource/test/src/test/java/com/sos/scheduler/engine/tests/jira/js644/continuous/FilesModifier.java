package com.sos.scheduler.engine.tests.jira.js644.continuous;

import static com.google.common.base.Preconditions.checkArgument;
import static java.lang.Math.random;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;

import org.apache.log4j.Logger;

import com.google.common.base.Charsets;
import com.google.common.collect.ImmutableList;
import com.google.common.io.Files;

final class FilesModifier {
    private static final Charset encoding = Charsets.UTF_8;
    private static final Logger logger = Logger.getLogger(FilesModifier.class);

    private final ImmutableList<File> files;

    FilesModifier(ImmutableList<File> files) {
        this.files = files;
    }

    void modifyRandom() {
        modify((int)(files.size() * random()));
    }

    private void modify(int index) {
        File f = files.get(index);
        logger.debug("modifyFile #"+index+" "+f);
        modifyFile(f);
    }

    private void modifyFile(File f) {
        try {
            checkArgument(f.exists(), "Datei fehlt: %s", f);
            Files.append(" ", f, encoding);
        } catch (IOException x) { throw new RuntimeException(x); }
    }

    int fileCount() {
        return files.size();
    }
}
