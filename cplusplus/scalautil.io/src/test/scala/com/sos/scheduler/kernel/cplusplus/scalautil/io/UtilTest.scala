package com.sos.scheduler.kernel.cplusplus.scalautil.io

import java.io._
import org.junit._
import org.junit.Assert._
import Util._


class UtilTest {
    @Test def testCloseQuietly {
        closeQuietly(new StringWriter)
    }

    @Test def testCloseQuietly2 {
        closeQuietly(null: StringWriter)
    }
}
