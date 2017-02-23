package com.sos.scheduler.engine.tests.jira.js1256

import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.scalautil.Closers._
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1256.JS1256IT._
import java.io.{File, RandomAccessFile}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * JS-1256 File_order_source can handle files bigger than 4GB.
 */
@RunWith(classOf[JUnitRunner])
final class JS1256IT extends FreeSpec with ScalaSchedulerTest {
  s"file_order_source with 4GB file - $FileSize bytes" in {
    withCloser { implicit closer ⇒
      val directory = testEnvironment.newFileOrderSourceDirectory()
      val bigFile = directory / "JS-1256.tmp"
      onClose { bigFile.delete() }
      allocateFile(bigFile, FileSize)
      withEventPipe { eventPipe ⇒
        scheduler executeXml JobChainElem(directory)
        eventPipe.next[OrderFinished](TestJobChainPath.orderKey(bigFile.getPath))
        bigFile should not be 'exists
      }
    }
  }
}

private object JS1256IT {
  private val FileSize = 0x100000001L
  assert(FileSize > Int.MaxValue)
  private val TestJobChainPath = JobChainPath("/test")

  private def JobChainElem(directory: File) =
    <job_chain name="test">
      <file_order_source directory={directory.getPath}/>
      <job_chain_node state="100" job="test"/>
      <file_order_sink state="END" remove="yes"/>
    </job_chain>

  private def allocateFile(file: File, size: Long): Unit = {
    autoClosing(new RandomAccessFile(file, "rw")) { _.setLength(size) }
    file.length shouldEqual size
  }
}
