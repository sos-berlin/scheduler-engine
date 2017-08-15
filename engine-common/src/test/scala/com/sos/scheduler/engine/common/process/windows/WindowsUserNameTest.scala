package com.sos.scheduler.engine.common.process.windows

import org.scalatest.FreeSpec

/**
  * @author Joacim Zschimmer
  */
final class WindowsUserNameTest extends FreeSpec {

  "equals" in {
    assert(WindowsUserName("a") != WindowsUserName("b"))
    assert(WindowsUserName("a") == WindowsUserName("A"))
    assert(WindowsUserName("å") == WindowsUserName("Å"))
  }
}
