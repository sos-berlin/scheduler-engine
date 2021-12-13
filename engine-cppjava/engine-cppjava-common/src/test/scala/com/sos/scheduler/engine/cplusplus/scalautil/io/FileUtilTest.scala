package com.sos.scheduler.engine.cplusplus.scalautil.io

import com.google.common.io.{Files, MoreFiles}
import com.sos.scheduler.engine.cplusplus.scalautil.io.FileUtil._
import java.io._
import java.nio.charset.Charset
import org.junit.Assert._
import org.junit._

final class FileUtilTest {

  @Test def testDeleteFile(): Unit = {
    val file = File.createTempFile("test", "tmp")
    deleteFile(file)
    assert(!file.exists)
  }

  @Test def testDeleteFile_directory(): Unit = {
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

  @Test def testWritingFileIfDifferent(): Unit = {
    val file = File.createTempFile("test","tmp")
    def f(w: Writer): Unit = { w.write("TEST\n") }
    assertTrue(writingFileIfDifferent(file, Charset.defaultCharset)(f))
    assertFalse(writingFileIfDifferent(file, Charset.defaultCharset)(f))
    file.delete
  }

  @Test def testListRecursive(): Unit = {
    val dir = File.createTempFile("test","tmp")
    dir.delete
    dir.mkdir()
    try {
      val subDirs = List("a", "a/b", "a/c", ".test")
      val paths = List("a/1", "a/b/ab1", "a/b/ignore", "a/b/ab2", "a/c/ac1", ".test/ignore")
      subDirs foreach { d => new File(dir, d).mkdir() }
      paths foreach { p => Files.touch(new File(dir, p)) }
      assertEquals(paths.toSet, listFilePathsRecursive(dir).toSet)
      assertEquals((paths filterNot { _ endsWith "ignore" }).toSet, listFilePathsRecursiveFiltered(dir){ _.getName != "ignore" } .toSet)
    } finally {
      MoreFiles.deleteDirectoryContents(dir.toPath)
      dir.delete()
    }
  }
}
