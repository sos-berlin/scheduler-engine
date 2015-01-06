package com.sos.scheduler.engine.playground.zschimmer.cpptoutf8

import java.io.File

private object Main {
  def main(args: Array[String]): Unit = {
    for (a <- args)
      CppToUtf8.convertFileOrDirectory(new File(a))
  }
}
