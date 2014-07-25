package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.common.scalautil.Logger
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class ProcessDescriptorTest extends FreeSpec {
  "fromXml" in {
    ProcessDescriptor.fromXml(<spooler><answer><process process_id="XY" pid="2222"/></answer></spooler>.toString()) shouldEqual
      ProcessDescriptor(processId = "XY", pid = 2222)
  }
}

private object ProcessDescriptorTest {
  private val logger = Logger(getClass)
}
