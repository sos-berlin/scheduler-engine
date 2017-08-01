package com.sos.scheduler.engine.common.process.windows

import scala.collection.immutable.Seq

/**
  * @author Joacim Zschimmer
  */
private[windows] object CommandLineConversion {

  // Here is how JDK does Windows quoting:
  // http://hg.openjdk.java.net/jdk8/jdk8/jdk/file/687fd7c7986d/src/windows/classes/java/lang/ProcessImpl.java

  private val ToBeQuoted = Set(' ', '"')

  def argsToCommandLine(args: Seq[String]): String = {
    val quotedArgs = quote(args.head.replace('/', '\\')) +:
      (args.tail map quote)
    quotedArgs mkString " "
  }

  private def quote(arg: String) = {
    require(!arg.contains('"'), "Windows command line argument must not contain a quote (\")")
    if (arg exists ToBeQuoted)
      '"' + arg + '"'
    else
      arg
  }
}
