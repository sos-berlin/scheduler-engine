package com.sos.scheduler.engine
package tests.jira.js856

import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/** JS-856 */
@RunWith(classOf[JUnitRunner])
class NewBehaviourJS856Test extends JS856Test {
  override val schedulerResourceNameMap = List("new-behaviour-scheduler.xml" -> "scheduler.xml")

  test("New behaviour: restore original order state") {
    expectFinishedParameters(List("a" -> "a-original"))
  }
}
