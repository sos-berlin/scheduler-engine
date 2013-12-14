package com.sos.scheduler.engine.common.utils

import com.sos.scheduler.engine.common.utils.Randoms._
import java.net.{BindException, ServerSocket}
import scala.sys.error

object FreeTcpPortFinder {

  val standardTcpPortRange = 10000 until 20000
  val alternateTcpPortRange = 20000 until 30000   // Für JS1052IT, der viele Ports belegt, um zufällige Überschneidung zu verhindern

  def findRandomFreeTcpPort(range: Range = standardTcpPortRange): Int =
    findFreeTcpPort(randomInts(range)) getOrElse error(s"No tcp port available in $range")

  def findFreeTcpPort(ports: TraversableOnce[Int]): Option[Int] =
    ports.toIterator find portIsFree

  private def portIsFree(port: Int) =
    try {
      val backlog = 1
      new ServerSocket(port, backlog).close()
      true
    } catch {
      case _: BindException => false
    }
}
