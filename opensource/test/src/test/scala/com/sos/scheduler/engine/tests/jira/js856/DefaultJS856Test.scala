package com.sos.scheduler.engine
package tests.jira.js856

import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/** JS-856 */
@RunWith(classOf[JUnitRunner])
class DefaultJS856Test extends JS856Test {
  test("Current default behaviour: keep order state") {
    expectFinishedParameters(List("a" -> "a-job", "b" -> "b-job"))
  }
}
