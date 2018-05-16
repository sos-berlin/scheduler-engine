package com.sos.scheduler.engine.cplusplus.scalautil.io

import org.junit._
import Util._

final class UtilTest {
    @Test def testCloseQuietly(): Unit = {
        val a = new A
        closeQuietly(a)
        assert(a.closed)
    }

    @Test def closeQuietlyShouldIgnoreException(): Unit = {
        suppressLogging(Util.getClass){ closeQuietly(null) }
    }

    private class A extends AutoCloseable {
        var closed = false
        def close(): Unit = {
            closed = true
        }
    }
}
