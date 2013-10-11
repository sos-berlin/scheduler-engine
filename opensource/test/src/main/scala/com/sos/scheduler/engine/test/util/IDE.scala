package com.sos.scheduler.engine.test.util

object IDE {
  def isRunningUnderIDE: Boolean = {
    val v = Option(System getProperty "sun.java.command") getOrElse ""
    v.startsWith("com.intellij.") || v.startsWith("org.jetbrains.")
  }

//  private def isRunningUnderMaven: Boolean =
//    System.getProperty("com.sos.scheduler.engine.test.underMaven") != null
}
