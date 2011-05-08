package com.sos.scheduler.engine.cplusplus.scalautil.io      // Kann gelegentlich verallgemeinert werden zu com.sos.scalautil.io

import java.io._
import org.apache.log4j.Logger


object Util {
    private val log = Logger.getLogger(getClass)

    private type HasClose = { def close() }
    
    /** Implementiert das Load Pattern mit close().
     * Aufruf: closingFinally( new InputStream ) { inputStream => code }.
     * B ist üblicherweise Unit.
    */
    def closingFinally[A <: HasClose, B](a: A)(function: A => B): B = {
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

    def closeQuietly[A <: HasClose](o: A) =
        try o.close()
        catch { case x: Exception => log.error(x.toString, x) }
}
