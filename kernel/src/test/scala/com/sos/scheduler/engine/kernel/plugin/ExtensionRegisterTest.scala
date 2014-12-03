package com.sos.scheduler.engine.kernel.plugin

import ExtensionRegisterTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class ExtensionRegisterTest extends FreeSpec {
  "ExtensionRegister" in {
    val testPlugin = new TestPlugin
    testPlugin.addExtension(TestExtension(11))
    intercept[IllegalStateException] { testPlugin.extensions }
    testPlugin.prepare()
    testPlugin.addExtension(TestExtension(22))
    testPlugin.extensions shouldEqual List(TestExtension(11), TestExtension(22))
    intercept[IllegalStateException] { testPlugin.addExtension(TestExtension(33)) }
  }
}

private object ExtensionRegisterTest {
  private case class TestExtension(x: Int)
  private class TestPlugin extends Plugin with ExtensionRegister[TestExtension]
}
