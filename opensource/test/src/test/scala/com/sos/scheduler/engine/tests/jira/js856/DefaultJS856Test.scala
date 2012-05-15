package com.sos.scheduler.engine.tests.jira.js856

import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import JS856Test.modifiedParameters

/** JS-856 */
@RunWith(classOf[JUnitRunner])
class DefaultJS856Test extends JS856Test("Current default behaviour: keep order state") {
  val finallyExpectedParameters = modifiedParameters
  val whenSuspendedExpectedParameters = modifiedParameters
}
