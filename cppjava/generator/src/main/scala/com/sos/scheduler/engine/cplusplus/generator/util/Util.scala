package com.sos.scheduler.engine.cplusplus.generator.util

import com.sos.scheduler.engine.cplusplus.generator.Configuration.defaultPrinter
import com.sos.scheduler.engine.cplusplus.scalautil.io.FileUtil._
import java.io.File
import java.io.Writer
import java.nio.charset.Charset


object Util {
    def inParentheses(a: Seq[_]) = a mkString ("(",", ", ")")

    def writingFileAndLog(file: File, encoding: Charset)(writeFunction: Writer => Unit) {
        def log() { defaultPrinter.println(file) }
        var ok = false
        try {
            writingFileIfDifferent(file, encoding)(writeFunction) match {
                case true => log()
                case false => //defaultPrinter.print(" (unchanged)")
            }
            ok = true
        }
        finally if (!ok) log()
    }


//    private def whenException[T](f: => T)(g: => Unit) = {
//        var ok = false
//        try {
//            f
//            ok = true
//        }
//        finally
//            if (!ok) g
//    }
}
