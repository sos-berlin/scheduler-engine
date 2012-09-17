package com.sos.scheduler.engine.test.util

import Sockets._
import java.net.ServerSocket
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
class SocketsTest extends FunSuite {
  private val backlog = 1

  test("findAvailablePort() should not return a used port number") {
    val aPort = findAvailablePort()
    val aSocket = new ServerSocket(aPort, backlog)
    try {
      val bPort = findAvailablePort()
      aPort should not equal (bPort)
      new ServerSocket(bPort, backlog).close()
    }
    finally {
      aSocket.close()
    }
  }

  test("findAvailablePort() should return randomly different port numbers") {
    val ports = for (i <- 0 until 100) yield findAvailablePort()
    ports.toSet.size should be > (10)
  }

  test("findAvailablePort() should throw an exception when no port is available") {
    intercept[RuntimeException] { findAvailablePort(10000 until 10000) }
  }

  test("portIsAvailable()") {
    val port = findAvailablePort()
    val socket = new ServerSocket(port, backlog)
    try portIsAvailable(port) should equal (false)
    finally socket.close()
    portIsAvailable(port) should equal (true)
  }
}

