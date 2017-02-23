package com.sos.scheduler.engine.test

import com.sos.jobscheduler.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.system.Files._
import com.sos.scheduler.engine.test.ProvidesTestDirectory._
import java.io.File

trait ProvidesTestDirectory extends HasCloser {

  protected lazy val testDirectory: File = {
    val file = System.getProperty(WorkDirectoryPropertyName) match {
      case null ⇒
        val result = new File(System.getProperty("java.io.tmpdir"), s"test-$testName")
        onClose { tryRemoveDirectoryRecursivly(testDirectory) }
        result
      case workDir ⇒
        new File(workDir).mkdir
        new File(workDir, testName)
    }
    file.getCanonicalFile
  }

  final def testName = testClass.getName

  protected def testClass: Class[_]
}

private object ProvidesTestDirectory {
  private val WorkDirectoryPropertyName = "com.sos.scheduler.engine.test.directory"
}
