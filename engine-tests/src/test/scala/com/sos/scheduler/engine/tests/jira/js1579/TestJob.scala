package com.sos.scheduler.engine.tests.jira.js1579

/**
  * @author Joacim Zschimmer
  */
final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    println(s"INVALID-->${JS1579IT.InvalidXmlCharacters.mkString}<--INVALID")
    false
  }
}
