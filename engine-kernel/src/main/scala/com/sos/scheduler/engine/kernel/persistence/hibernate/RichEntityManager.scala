package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.jobscheduler.base.utils.ScalaUtils
import com.sos.jobscheduler.base.utils.ScalaUtils.implicitClass
import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.scalautil.Collections.implicits._
import java.sql.{Connection, PreparedStatement}
import javax.persistence.EntityManager
import org.hibernate.jdbc.Work
import scala.collection.immutable
import scala.language.implicitConversions
import scala.reflect.ClassTag


class RichEntityManager(val delegate: EntityManager) extends AnyVal {

  def findOption[E : ClassTag](key: AnyRef): Option[E] =
    findOption(key, implicitClass[E])

  def findOption[E](key: AnyRef, clas: Class[E]): Option[E] =
    Option(delegate.find(clas, key))

  def fetchOption[E <: AnyRef : ClassTag](queryString: String, arguments: Iterable[(String, Any)] = Nil): Option[E] =
    fetchClassOption[E](queryString, implicitClass[E], arguments)

  def fetchClassOption[E <: AnyRef](queryString: String, clas: Class[E], arguments: Iterable[(String, Any)] = Nil): Option[E] = {
    val i = fetchClassSeq(queryString, clas, arguments).iterator
    if (i.hasNext) Some(i.next()) ensuring { _ ⇒ !i.hasNext }
    else None
  }

  def fetchSeq[A <: AnyRef : ClassTag](queryString: String, arguments: Iterable[(String, Any)] = Nil): immutable.Seq[A] =
    fetchClassSeq[A](queryString, implicitClass[A], arguments)

  def fetchClassSeq[A <: AnyRef](queryString: String, clas: Class[A], arguments: Iterable[(String, Any)]): immutable.Seq[A] = {
    val q = delegate.createQuery(queryString, clas)
    for ((name, value) <- arguments) q.setParameter(name, value)
    q.getResultList.toImmutableSeq
  }

  def useJDBCPreparedStatement[A](sql: String)(f: PreparedStatement ⇒ A) = {
    useJDBCConnection { connection ⇒
      autoClosing(connection.prepareStatement(sql)) { preparedStatement ⇒
        f(preparedStatement)
      }
    }
  }

  def useJDBCConnection[A](f: Connection ⇒ A): A = {
    // Geht nicht mit Hibernate 4.1.7 (aber mit EclipseLink): f(em.unwrap(classOf[java.sql.Connection]))
    var result: Option[A] = None
    delegate.unwrap(classOf[org.hibernate.Session]).doWork(new Work {
      def execute(connection: Connection): Unit = {
        result = Some(f(connection))

      }
    })
    result.get
  }
}

object RichEntityManager {
  implicit def toRichEntityManager(e: EntityManager): RichEntityManager = new RichEntityManager(e)
}
