package com.sos.scheduler.engine.common.utils

import com.sos.scheduler.engine.common.utils.Randoms._
import java.net.{BindException, ServerSocket}
import scala.sys.error

object FreeTcpPortFinder {

  def findRandomFreePort(range: Range): Int =
    findFreePort(randomInts(range)) getOrElse error(s"No tcp port available in $range")

  def findFreePort(ports: TraversableOnce[Int]): Option[Int] =
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
