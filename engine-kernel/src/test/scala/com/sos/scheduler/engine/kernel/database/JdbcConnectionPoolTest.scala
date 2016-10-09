package com.sos.scheduler.engine.kernel.database

import com.sos.scheduler.engine.common.concurrent.ParallelismCounter
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.typesafe.config.ConfigFactory
import java.nio.file.Files
import java.nio.file.Files.createTempDirectory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global
import scala.math.min

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JdbcConnectionPoolTest extends FreeSpec {

  "test" in {
    val tmpDir = createTempDirectory("JdbcConnectionPoolTest-")
    val cppDatabaseProperties = CppDatabaseProperties(url = s"jdbc:h2:$tmpDir/database", driverClassName = Some("org.h2.Driver"))
    val maximumPoolSize = min(7, sys.runtime.availableProcessors)
    val config = ConfigFactory.parseString(s"hikari { maximumPoolSize = $maximumPoolSize }")
    autoClosing(new JdbcConnectionPool(config, () ⇒ cppDatabaseProperties)) { connectionPool ⇒
      for (_ ← 1 to 3) {
        val aFuture = connectionPool.future { connection ⇒
          val stmt = connection.createStatement()
          stmt.execute("create table test (i integer, s varchar(100))")
          stmt.execute("insert into test values (0, 'TEST')")
          connection.commit()
        }
        aFuture await 60.s
        val n = 100
        val count = new ParallelismCounter
        val futures = for (_ ← 1 to n) yield
          connectionPool.future { connection ⇒
            count {
              val result = autoClosing(connection.createStatement()) { stmt ⇒
                stmt.execute("update test set i = i + 1")
                val resultSet = stmt.executeQuery("select i from test")
                resultSet.next()
                resultSet.getInt("i")
              }
              connection.commit()
              result
            }
          }
        assert((futures await 30.s).toSet == (1 to n).toSet)
        assert(connectionPool.poolSize == maximumPoolSize)
        assert(count.total == n)
        assert(count.maximum == connectionPool.poolSize)
        connectionPool.future { connection ⇒
          connection.createStatement().execute("drop table test")
          connection.commit()
        } await 60.s
      }
    }
    for (file ← Files.list(tmpDir)) Files.delete(file)
    Files.delete(tmpDir)
  }
}
