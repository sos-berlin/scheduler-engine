package com.sos.scheduler.engine.tests.scheduler.job.stdout

import com.sos.scheduler.engine.tests.scheduler.job.stdout.TestJob._
import java.lang.Thread.sleep

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  System.out.println(StartStderrMessage)
  System.err.println(StartStdoutMessage)

  override def spooler_process() = {
    sleep(20*1000)
    false
  }

  override def spooler_exit() = {
    System.err.println(ExitStdoutMessage)
    System.out.println(ExitStderrMessage)
  }
}

private[stdout] object TestJob {
  val StartStdoutMessage = "START-STDOUT"
  val StartStderrMessage = "START-STDOUT"
  val ExitStdoutMessage = "EXIT-STDOUT"
  val ExitStderrMessage = "EXIT-STDOUT"
}
