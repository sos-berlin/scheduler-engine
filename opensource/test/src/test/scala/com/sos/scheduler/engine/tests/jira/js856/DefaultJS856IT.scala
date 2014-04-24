package com.sos.scheduler.engine.tests.jira.js856

import JS856IT.modifiedParameters
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/** JS-856 */
@RunWith(classOf[JUnitRunner])
final class DefaultJS856IT extends JS856IT("Current default behaviour: keep order state") {
  val finallyExpectedParameters = modifiedParameters
  val whenSuspendedExpectedParameters = modifiedParameters
}
