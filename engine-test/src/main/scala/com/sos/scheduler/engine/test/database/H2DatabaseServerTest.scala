package com.sos.scheduler.engine.test.database

import com.google.common.io.Files.createTempDir
import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.HasCloser
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.test.database.H2DatabaseServer.Configuration
import java.nio.file.Files
import java.sql.DriverManager
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class H2DatabaseServerTest extends FreeSpec with HasCloser {

  "Start and stop database" in {
    val dbDirectory = createTempDir()
    try {
      val conf = new Configuration {
        def directory = dbDirectory
        lazy val tcpPort = findRandomFreeTcpPort()
      }
      autoClosing(new H2DatabaseServer(conf)) { server ⇒
        server.start()
        autoClosing(DriverManager.getConnection(conf.jdbcUrl)) { connection ⇒
          connection.createStatement().executeUpdate("create table test (a int)")
        }
        dbDirectory.listFiles shouldEqual List(server.file)
        intercept[Exception] { server.start() }
        server.stop()
        server.start()
      }
    }
    finally for (f ← dbDirectory.listFiles :+ dbDirectory) Files.delete(f)
  }
}
