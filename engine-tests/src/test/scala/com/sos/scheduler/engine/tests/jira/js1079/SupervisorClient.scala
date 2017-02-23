package com.sos.scheduler.engine.tests.jira.js1079

import com.sos.scheduler.engine.common.client.SchedulerTcpConnection
import com.sos.jobscheduler.common.scalautil.xmls.SafeXML
import java.io.ByteArrayInputStream
import java.net.{DatagramPacket, InetSocketAddress, DatagramSocket}
import org.scalatest.Matchers._

private[js1079] final class SupervisorClient(val index: Int, udpSocket: DatagramSocket, serverAddress: InetSocketAddress)
    extends AutoCloseable {

  require(udpSocket.isBound)

  private val schedulerId = s"client-$index"
  private val myTcpPort = 10000 + index
  private val connection = new SchedulerTcpConnection(serverAddress)

  def connect(): Unit = {
    connection.connect()
  }

  def close(): Unit = {
    try connection.close()
    finally udpSocket.close()
  }

  def registerMe(): Unit = {
    connection sendAndReceiveXML <register_remote_scheduler tcp_port={myTcpPort.toString} udp_port={udpSocket.getLocalPort.toString} scheduler_id={schedulerId} version="1.5"/>
  }

  def fetchUpdatedFiles(): Unit = {
    connection sendAndReceiveXML <supervisor.remote_scheduler.configuration.fetch_updated_files/>
  }

  def expectUdp(expectedElem: xml.Elem): Unit = {
    val b = new Array[Byte](1000)
    val p = new DatagramPacket(b, b.length)
    udpSocket setSoTimeout 15 * 1000
    udpSocket receive p
    val receivedElem = SafeXML.load(new ByteArrayInputStream(b, 0, p.getLength))
    receivedElem should equal (expectedElem)
  }
}
