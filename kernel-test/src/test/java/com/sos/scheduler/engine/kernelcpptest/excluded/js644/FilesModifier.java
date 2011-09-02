/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import com.google.common.base.Charsets;
import com.google.common.collect.Iterators;
import com.google.common.io.Files;
import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Collection;
import java.util.Iterator;
import org.apache.log4j.Logger;


public class FilesModifier {
    private static final Charset encoding = Charsets.UTF_8;
    private static final Logger logger = Logger.getLogger(FilesModifier.class);

    private final Collection<File> files;
    private Iterator<File> iterator = Iterators.emptyIterator();


    public FilesModifier(Collection<File> files) {
        this.files = files;
    }


    public void modifyNext() {
        if (!iterator.hasNext())
            iterator = files.iterator();
        modifyFile(iterator.next());
    }


    private void modifyFile(File f) {
        try {
            logger.info("modifyFile " + f);
            assert f.exists();
            Files.append("<!-- -->\n", f, encoding);
        } catch (IOException x) { throw new RuntimeException(x); }
    }

    
    public int getFileCount() {
        return files.size();
    }
}
