package com.sos.scheduler.engine.newkernel.job

import java.io.{BufferedReader, InputStreamReader, InputStream}
import java.nio.charset.Charset
import org.slf4j.Logger

class InputStreamLogger(in: InputStream, charset: Charset, logger: Logger) extends Runnable {
  private val reader = new BufferedReader(new InputStreamReader(in, charset))

  //TODO Mit NIO, Channel und Selector in einem Thread für alle Ausgaben implementieren
  //Das scheint nicht möglich zu sein. Dann wie in C++ in Dateien schreiben und diese mit Selector Lesen.
  // http://stackoverflow.com/questions/1033653

  def run(): Unit = {
    while (true) {
      val line = reader.readLine()
      if (line == null) return
      logger info line
    }
  }
}
