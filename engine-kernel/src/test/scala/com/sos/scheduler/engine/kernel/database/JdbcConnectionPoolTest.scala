package com.sos.scheduler.engine.kernel.database

import com.sos.jobscheduler.common.concurrent.ParallelismCounter
import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.typesafe.config.ConfigFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.immutable.Seq
import scala.concurrent.ExecutionContext.Implicits.global
import scala.math.min

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JdbcConnectionPoolTest extends FreeSpec {

  "Parallelism" in {
    val cppDatabaseProperties = CppDatabaseProperties(url = s"jdbc:h2:mem:JdbcConnectionPoolTest")
    val poolSize = min(7, sys.runtime.availableProcessors)
    val config = ConfigFactory.parseString(s"hikari { maximumPoolSize = $poolSize }")
    autoClosing(new JdbcConnectionPool(config, () ⇒ cppDatabaseProperties)) { connectionPool ⇒
      for (_ ← 1 to 3) {
        connectionPool.future { connection ⇒
          val stmt = connection.createStatement()
          stmt.execute("create table test (i integer, s varchar(100))")
          stmt.execute("insert into test values (0, 'TEST')")
          connection.commit()
        } await 30.s
        val count = new ParallelismCounter
        val n = 100
        val numbers: Seq[Int] = (for (_ ← 1 to n) yield
          connectionPool.future { connection ⇒
            count {
              val result = autoClosing(connection.createStatement()) { stmt ⇒
                stmt.execute("update test set i = i + 1")
                val resultSet = stmt.executeQuery("select i from test")
                resultSet.next()
                resultSet.getInt("i")
              }
              connection.commit()
              sleep(10.ms)  // Delay to ensure parallelism
              result
            }
          }) await 30.s
        assert(numbers.toSet == (1 to n).toSet)
        assert(connectionPool.maximumPoolSize == poolSize)
        assert(count.total == n)
        assert(count.maximum == connectionPool.maximumPoolSize, ", parallelism != maximumPoolSize")
        connectionPool.future { connection ⇒
          connection.createStatement().execute("drop table test")
          connection.commit()
        } await 60.s
      }
    }
  }
}
