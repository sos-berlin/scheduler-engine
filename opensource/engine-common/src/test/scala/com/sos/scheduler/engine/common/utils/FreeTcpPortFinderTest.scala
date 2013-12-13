package com.sos.scheduler.engine.common.utils

import FreeTcpPortFinder._
import java.net.ServerSocket
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class FreeTcpPortFinderTest extends FunSuite {

  test("findRandomFreePort") {
    for (i <- 1 to 1000) {
      findRandomFreeTcpPort(20000 until 30000) should (be >= 20000 and be <= 30000)
    }
  }

  test("findRandomFreePort with empty list should throw exception") {
    val portNumber = findRandomFreeTcpPort(20000 until 30000)
    val socket = new ServerSocket(portNumber, 1)
    intercept[Exception] { findRandomFreeTcpPort(portNumber to portNumber) }
    socket.close()
    findRandomFreeTcpPort(portNumber to portNumber)
  }

  test("findRandomFreePort with no available port should throw exception") {
    val emptyRange = 20000 until 20000
    emptyRange.size should equal (0)
    intercept[Exception] { findRandomFreeTcpPort(emptyRange) }
  }
}
