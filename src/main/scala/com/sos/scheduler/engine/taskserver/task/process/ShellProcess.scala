package com.sos.scheduler.engine.taskserver.task.process

import com.sos.scheduler.engine.common.scalautil.HasCloser

/**
 * @author Joacim Zschimmer
 */
final class ShellProcess(process: Process) extends HasCloser {

  override def onClose(body: ⇒ Unit): Unit = super.onClose(body)

  def waitFor(): Int = process.waitFor()
}
