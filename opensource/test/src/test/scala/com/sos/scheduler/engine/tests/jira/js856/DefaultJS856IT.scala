package com.sos.scheduler.engine.tests.jira.js856

import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import JS856IT.modifiedParameters

/** JS-856 */
@RunWith(classOf[JUnitRunner])
class DefaultJS856IT extends JS856IT("Current default behaviour: keep order state") {
  val finallyExpectedParameters = modifiedParameters
  val whenSuspendedExpectedParameters = modifiedParameters
}
