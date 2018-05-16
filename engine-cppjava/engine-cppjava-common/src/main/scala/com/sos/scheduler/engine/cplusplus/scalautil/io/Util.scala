package com.sos.scheduler.engine.cplusplus.scalautil.io

import org.slf4j.LoggerFactory

// Kann gelegentlich verallgemeinert werden zu com.sos.scalautil.io

object Util {
    private val logger = LoggerFactory.getLogger(getClass)

    /** Implementiert das Load Pattern mit close().
     * Aufruf: closingFinally( new InputStream ) { inputStream => code }.
     * B ist üblicherweise Unit.
     */
    def closingFinally[A <: AutoCloseable, B](a: A)(function: A => B): B = {
        if (a == null)  throw new NullPointerException("closingFinally: object is null")
        var ok = false
        try {
            val b = function(a)
            a.close()
            ok = true
            b
        }
        finally if (!ok) closeQuietly(a)
    }

    def closeQuietly[A <: AutoCloseable](o: A): Unit = {
        try o.close()
        catch { case x: Exception => logger.error(x.toString, x) }
    }

    def suppressLogging[A](c: Class[_])(f: => A): Unit = {
//        val lg = Logger.getLogger(Util.getClass)
//        val originalLevel = lg.getLevel
//        lg.setLevel(Level.OFF)
//        try f
//        finally lg.setLevel(originalLevel)
    }
}
