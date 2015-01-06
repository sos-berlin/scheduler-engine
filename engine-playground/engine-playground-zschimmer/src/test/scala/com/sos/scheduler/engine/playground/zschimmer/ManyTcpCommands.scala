package com.sos.scheduler.engine.playground.zschimmer

import com.google.common.io.ByteStreams
import java.io.InputStream
import java.net.Socket

object ManyTcpCommands {
  def main(args: Array[String]): Unit = {
    val schedulerConnection = new SchedulerConnection("127.0.0.1", 4001)
    try {
      while (true) {
        val xml = s"<?xml version='1.0' encoding='ISO-8859-1'?><check_folders/>"
        val response = schedulerConnection.sendAndReceive(xml.getBytes("ISO-8859-1"))
        System.err.println(new String(response, "ISO-8859-1"))
      }
    } finally
      schedulerConnection.close()
  }

  private final class SchedulerConnection(host: String, port: Int) { //extends AutoClosing {
    val socket = new Socket(host, port)
    val outputStream = socket.getOutputStream
    private val inputStream = socket.getInputStream

    def close(): Unit = {
      socket.close()
    }

    def sendAndReceive(data: Array[Byte]): Array[Byte] = {
      outputStream.write(data)
      ByteStreams.toByteArray(newResponseInputStream())
    }

    def newResponseInputStream() = new InputStream {
      private var eof = false

      def read() =
        if (eof) -1
        else
          inputStream.read() match {
            case '\u0000' => eof = true; -1
            case c => c
          }
    }
  }
}
