package com.sos.scheduler.engine.kernel.database

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import java.lang.Math.max
import java.nio.file.Files
import java.nio.file.Files.createTempDirectory
import java.util.concurrent.atomic.AtomicInteger
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JdbcConnectionPoolTest extends FreeSpec {

  "test" in {
    val tmpDir = createTempDirectory("JdbcConnectionPoolTest-")
    val cppDatabaseProperties = CppDatabaseProperties(url = s"jdbc:h2:$tmpDir/database", driverClassName = Some("org.h2.Driver"))
    autoClosing(new JdbcConnectionPool(() ⇒ cppDatabaseProperties)) { connectionPool ⇒
      val aFuture = connectionPool.transactionFuture { connection ⇒
        val stmt = connection.createStatement()
        stmt.execute("create table test (i integer, s varchar(100))")
        stmt.execute("insert into test values (0, 'TEST')")
        connection.commit()
      }
      aFuture await 30.s
      val n = 100
      val parallel = new AtomicInteger
      var maximum = 0
      val lock = new Object
      val futures = for (_ ← 1 to n) yield
        connectionPool.transactionFuture { connection ⇒
          lock.synchronized {
            val i = parallel.incrementAndGet()
            maximum = max(maximum, i)
          }
          val result = autoClosing(connection.createStatement()) { stmt ⇒
            stmt.execute("update test set i = i + 1")
            val resultSet = stmt.executeQuery("select i from test")
            resultSet.next()
            resultSet.getInt("i")
          }
          connection.commit()
          parallel.decrementAndGet()
          result
        }
      assert((futures await 30.s).toSet == (1 to n).toSet)
      assert(parallel.get == 0)
      assert(connectionPool.poolSize == 10)
      assert(maximum == connectionPool.poolSize)
    }
    for (file ← Files.list(tmpDir)) Files.delete(file)
    Files.delete(tmpDir)
  }
}
