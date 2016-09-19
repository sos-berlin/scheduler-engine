package com.sos.scheduler.engine.tests.jira.js1291

/**
 * @author Joacim Zschimmer
 */
sealed abstract class StdoutApiJob extends sos.spooler.Job_impl {

  private var stepNumber = 0

  override def spooler_process() = {
    val order = spooler_task.order
    val orderId = order.id
    val jobPath = spooler_job.name
    System.out.println(s"STDOUT LINE STEP FOR $orderId, JOB $jobPath")
    System.out.println(s"STDERR LINE STEP FOR $orderId, JOB $jobPath")
    stepNumber < 2
  }
}

final class StdoutApi1Job extends StdoutApiJob
final class StdoutApi2Job extends StdoutApiJob
