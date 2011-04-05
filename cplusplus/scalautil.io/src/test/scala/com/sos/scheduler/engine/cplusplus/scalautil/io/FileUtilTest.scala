package com.sos.scheduler.engine.cplusplus.scalautil.io

import java.io._
import java.nio.charset.Charset
import org.apache.commons.io.FileUtils
import org.junit._
import org.junit.Assert._
import FileUtil._


class FileUtilTest {
    @Test def testDeleteFile {
        val file = File.createTempFile("test", "tmp")

        deleteFile(file)
        assert(!file.exists)
    }


    @Test def testDeleteFile_directory {
        val file = File.createTempFile("test", "tmp")
        deleteFile(file)
        file.mkdir
        val nestedFile = new File(file,"test.tmp")
        nestedFile.createNewFile

        assert(
            try { deleteFile(file); false }
            catch { case x: RuntimeException => nestedFile.delete; file.delete; true })

        assert(!file.exists)    // Alles aufgeräumt?
    }


    @Test def testWritingFileIfDifferent {
        val file = File.createTempFile("test","tmp")
        def f(w: Writer) { w.write("TEST\n") }
        assertTrue(writingFileIfDifferent(file, Charset.defaultCharset)(f))
        assertFalse(writingFileIfDifferent(file, Charset.defaultCharset)(f))
        file.delete
    }

    @Test def testListRecursive {
        val dir = File.createTempFile("test","tmp")
        dir.delete
        dir.mkdir()
        try {
            val subDirs = List("a", "a/b", "a/c", ".test")
            val paths = List("a/1", "a/b/ab1", "a/b/ignore", "a/b/ab2", "a/c/ac1", ".test/ignore")
            subDirs foreach { d => new File(dir, d).mkdir() }
            paths foreach { p => FileUtils.touch(new File(dir, p)) }
            assertEquals(paths.toSet, listFilePathsRecursive(dir).toSet)
            assertEquals(paths filter { !_.endsWith("ignore") } toSet, listFilePathsRecursiveFiltered(dir){_.getName != "ignore"} toSet)
        }
        finally FileUtils.deleteDirectory(dir)
    }
}
