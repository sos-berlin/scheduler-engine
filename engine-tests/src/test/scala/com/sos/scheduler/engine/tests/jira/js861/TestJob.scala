package com.sos.scheduler.engine.tests.jira.js861

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.tests.jira.js861.TestJob._
import java.nio.charset.CodingErrorAction.IGNORE
import java.nio.charset.StandardCharsets.US_ASCII
import scala.io.Codec

/**
  * @author Joacim Zschimmer
  */
final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    println(s"THIS IS THE JOB /${spooler_job.name}")
    spooler_log.info(execute("""C:\Windows\System32\ping.exe""", "-n", "1", "127.0.0.1"))
    spooler_log.info(s"TEST-USERNAME=SELF-TEST")
    spooler_log.info(s"TEST-USERNAME=$whoami")
    spooler_task.order.params.set_value("JOB-VARIABLE", "JOB-VALUE")
    false
  }
}

private[js861] object TestJob {
  def whoami = execute("""C:\Windows\System32\whoami.exe""")

  private def execute(args: String*): String = {
    val process = new ProcessBuilder(args: _*).start()
    val codec = Codec(US_ASCII).onMalformedInput(IGNORE).onUnmappableCharacter(IGNORE)
    val output = autoClosing(io.Source.fromInputStream(process.getInputStream)(codec)) { source ⇒
      source.getLines() mkString "\n"
    }
    process.waitFor()
    output
  }
}