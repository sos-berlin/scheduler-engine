package com.sos.scheduler.engine.kernelcpptest.stress.showstate

import java.io._
import java.net._
import java.nio.charset.Charset
import org.junit._
import com.sos.scheduler.engine.cplusplus.scalautil.io.Util._
import com.sos.scheduler.engine.kernelcpptest.ScalaSchedulerTest

final class ShowStateStressTest extends ScalaSchedulerTest {
    import ShowStateStressTest._
    
    //@Ignore
    @Test def test1() {
        controller.startScheduler("-e", "-log-level=warn")
        closingFinally(new Connection(new InetSocketAddress("localhost", scheduler.getTcpPort))) { connection =>
            1 to 1000 foreach { i => connection.sendAndReceive(emptyCommand) }
        }
    }
}

object ShowStateStressTest {
    private def emptyCommand = (  //<params/>.toString
        <commands>
            <subsystem.show what="statistics"/>
            <show_state what="folders cluster remote_schedulers schedules" subsystems="lock schedule process_class folder"/>
        </commands>
    ).toString()
    
    def main(args: Array[String]) {
        def inetSocketAddress(a: String) = {
            val b = a split ':'
            new InetSocketAddress(b(0), b(1).toInt)
        }

        val connection = new Connection(inetSocketAddress(args(0)))
        1 to Int.MaxValue foreach { _ => connection.sendAndReceive(emptyCommand) }
    }

    private class Connection(val address: InetSocketAddress) {
        private def encoding = Charset.forName("UTF-8")     //TODO Statt XML.toString besser XML direkt nach OutputStream schreiben lassen
        private val socket = new Socket
        socket.connect(address)
        private val writer = new OutputStreamWriter(socket.getOutputStream, encoding)
        private val reader = new SplitReader(new InputStreamReader(socket.getInputStream), '\0')

        def close() {
            socket.close()
        }

        def sendAndReceive(s: String): String = {
            writer.write(s)
            writer.flush()
            reader.read()
        }
    }

    private class SplitReader(reader: Reader, separator: Char) extends Closeable {
        def close() {
          reader.close()
        }

        def read(): String = {
            val result = new StringBuilder
            var c = reader.read()
            while (c != -1  &&  c.toChar != separator) {
                result.append(c.toChar)
                c = reader.read()
            }
            result.toString()
        }
    }
}
