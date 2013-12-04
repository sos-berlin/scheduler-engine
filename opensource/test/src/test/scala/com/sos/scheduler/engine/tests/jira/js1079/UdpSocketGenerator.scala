package com.sos.scheduler.engine.tests.jira.js1079

import java.net.{BindException, DatagramSocket}

private[js1079] class UdpSocketGenerator {
  @volatile private var udpPort = 10000

  def apply(): DatagramSocket =
    synchronized {
      var result: DatagramSocket = null
      while (result == null) {
        udpPort += 1
        try result = new DatagramSocket(udpPort)
        catch { case e: BindException if udpPort < 0x10000 => }
      }
      result
    }
}
