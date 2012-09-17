package com.sos.scheduler.engine.test.util

import java.net.{BindException, ServerSocket}
import scala.util.Random

object Sockets {
  /** Race-Condition möglich: Der gefundene Port kann kurz nach dem Aufruf belegt sein. */
  def findAvailablePort(): Int = {
    val offset = (1000 * new Random().nextFloat).asInstanceOf[Int]
    findAvailablePort(23000 + offset until 24000 + offset)
  }

  /** Race-Condition möglich: Der gefundene Port kann kurz nach dem Aufruf belegt sein. */
  def findAvailablePort(ports: Iterable[Int]): Int = ports find portIsAvailable getOrElse
      sys.error("No tcp port is avaible in "+ ports)

  def portIsAvailable(port: Int) =
    try {
      val backlog = 1
      new ServerSocket(port, backlog).close()
      true
    } catch {
      case x: BindException => false
    }
}