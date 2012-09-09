package com.sos.scheduler.engine.tests.jira.js856

import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import JS856IT.originalParameters

/** JS-856 */
@RunWith(classOf[JUnitRunner])
class NewBehaviourJS856IT extends JS856IT("New behaviour: restore original order state") {
  override val schedulerResourceNameMap = List("new-behaviour-scheduler.xml" -> "scheduler.xml")
  val finallyExpectedParameters = originalParameters
  val whenSuspendedExpectedParameters = Map("a" -> "a-job", "b" -> "b-job")
}
