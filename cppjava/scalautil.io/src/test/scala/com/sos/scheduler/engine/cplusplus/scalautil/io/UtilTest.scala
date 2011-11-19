package com.sos.scheduler.engine.cplusplus.scalautil.io

import org.junit._
import Util._

final class UtilTest {
    @Test def testCloseQuietly() {
        val a = new A
        closeQuietly(a)
        assert(a.closed)
    }

    @Test def closeQuietlyShouldIgnoreException() {
        suppressLogging(Util.getClass){ closeQuietly(null) }
    }

    private class A {
        var closed = false
        def close() {
            closed = true
        }
    }
}
