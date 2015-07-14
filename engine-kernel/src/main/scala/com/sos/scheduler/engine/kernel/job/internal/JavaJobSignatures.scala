package com.sos.scheduler.engine.kernel.job.internal

/**
 * @author Joacim Zschimmer
 */
object JavaJobSignatures {
  val SpoolerExitSignature = "spooler_exit()V"  // V: returns Unit
  val SpoolerOpenSignature = "spooler_open()Z"
  val SpoolerOnSuccessSignature = "spooler_on_success()V"
  val SpoolerOnErrorSignature = "spooler_on_error()V"
}
