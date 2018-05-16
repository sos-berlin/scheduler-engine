package com.sos.scheduler.engine.tests.stress.showstate

import com.google.common.base.Charsets
import com.sos.scheduler.engine.cplusplus.scalautil.io.Util._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.test.SchedulerTest
import java.io._
import java.net._
import org.junit._

final class ShowStateStressIT extends SchedulerTest {

  import ShowStateStressIT._

  @Ignore //TODO Test kann versagen, wenn Portnummer in scheduler.xml schon vergegeben ist.
  @Test def test1(): Unit = {
    controller.activateScheduler()
    closingFinally(new Connection(new InetSocketAddress("localhost", instance(classOf[SchedulerConfiguration]).tcpPort))) { connection =>
        for (i <- 1 to 1000) connection.sendAndReceive(emptyCommand)
    }
    controller.terminateScheduler()
  }
}

object ShowStateStressIT {
  private def emptyCommand = //<params/>.toString
      <commands>
          <subsystem.show what="statistics"/>
          <show_state what="folders cluster remote_schedulers schedules" subsystems="lock schedule process_class folder"/>
      </commands>
      .toString()

  def main(args: Array[String]): Unit = {
    def inetSocketAddress(a: String) = {
      val b = a split ':'
      new InetSocketAddress(b(0), b(1).toInt)
    }

    val connection = new Connection(inetSocketAddress(args(0)))
    for (i <- 1 to Int.MaxValue) connection.sendAndReceive(emptyCommand)
  }

  private class Connection(val address: InetSocketAddress) extends AutoCloseable {
    private def encoding = Charsets.UTF_8
    private val socket = new Socket
    socket.connect(address)
    private val writer = new OutputStreamWriter(socket.getOutputStream, encoding)
    private val reader = new SplitReader(new InputStreamReader(socket.getInputStream), '\u0000')

    def close(): Unit = {
      socket.close()
    }

    def sendAndReceive(s: String): String = {
      writer.write(s)
      writer.flush()
      reader.read()
    }
  }

  private class SplitReader(reader: Reader, separator: Char) extends Closeable {
    def close(): Unit = {
      reader.close()
    }

    def read(): String = {
      val result = new StringBuilder
      var c = reader.read()
      while (c != -1 && c.toChar != separator) {
        result.append(c.toChar)
        c = reader.read()
      }
      result.toString()
    }
  }
}
