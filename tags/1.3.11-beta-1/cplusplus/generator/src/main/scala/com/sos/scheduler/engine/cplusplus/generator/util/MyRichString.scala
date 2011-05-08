package com.sos.scheduler.engine.cplusplus.generator.util


class MyRichString(string: String) {
    def when(b: Boolean) = if (b) string  else ""
    def ?(b: Boolean) = when(b)
}


object MyRichString {
    implicit def myRichStringOfString(s: String) = new MyRichString(s)

    def noneIfEmpty(s: String): Option[String] = if (s.isEmpty) None  else Some(s)
}
