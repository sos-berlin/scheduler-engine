package com.sos.scheduler.engine.tests.jira.js1079

import com.google.common.base.Charsets._
import com.sos.scheduler.engine.common.client.SchedulerTcpConnection
import com.sos.scheduler.engine.common.utils.SosAutoCloseable
import java.net.{DatagramPacket, InetSocketAddress, DatagramSocket}
import org.scalatest.matchers.ShouldMatchers._

private[js1079] final class SupervisorClient(val index: Int, udpSocket: DatagramSocket, serverAddress: InetSocketAddress) extends SosAutoCloseable {
  require(udpSocket.isBound)

  private val schedulerId = s"SCHEDULER-$index"
  private val myTcpPort = 10000 + index
  private val connection = new SchedulerTcpConnection(serverAddress)

  def connect() {
    connection.connect()
  }

  def close() {
    try connection.close()
    finally udpSocket.close()
  }

  def registerMe() {
    connection sendAndReceiveXML <register_remote_scheduler tcp_port={myTcpPort.toString} udp_port={udpSocket.getLocalPort.toString} scheduler_id={schedulerId} version="1.5"/>
  }

  def fetchUpdatedFiles() {
    connection sendAndReceiveXML <supervisor.remote_scheduler.configuration.fetch_updated_files/>
  }

  def expectUdp(expectedMessage: String) {
    val b = new Array[Byte](1000)
    val p = new DatagramPacket(b, b.length)
    udpSocket.setSoTimeout(10 * 1000)
    udpSocket.receive(p)
    val receivedString = new String(b, 0, p.getLength, UTF_8)
    receivedString should equal (expectedMessage)
  }
}
