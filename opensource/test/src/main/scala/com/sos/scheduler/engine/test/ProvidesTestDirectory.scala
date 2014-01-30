package com.sos.scheduler.engine.test

import ProvidesTestDirectory._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.system.Files._
import com.sos.scheduler.engine.test.util.TestUtils.currentTestClass
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
        new File(workDir, testName)
    }

  private def testName =
    testClass.getName

  protected def testClass: Class[_] =
    currentTestClass
}

private object ProvidesTestDirectory {
  private val workDirectoryPropertyName = "com.sos.scheduler.engine.test.directory"
}
