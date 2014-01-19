package com.sos.scheduler.engine.test

import ProvidesTestDirectory._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.system.Files._
import java.io.File

trait ProvidesTestDirectory extends HasCloser {

  protected lazy val testDirectory: File =
    System.getProperty(workDirectoryPropertyName) match {
      case null =>
        val result = new File(System.getProperty("java.io.tmpdir"), s"test-$testName")
        onClose { tryRemoveDirectoryRecursivly(testDirectory) }
        result
      case workDir =>
        new File(workDir).mkdir
        val result = new File(workDir, testName)
        makeCleanDirectory(result)
        result
    }

  private def testName =
    testClass.getName

  protected def testClass: Class[_] =
    getClass
}

private object ProvidesTestDirectory {
  private val workDirectoryPropertyName = "com.sos.scheduler.engine.test.directory"

  private def makeCleanDirectory(directory: File) {
    makeDirectory(directory)
    removeDirectoryContentRecursivly(directory)
  }
}
