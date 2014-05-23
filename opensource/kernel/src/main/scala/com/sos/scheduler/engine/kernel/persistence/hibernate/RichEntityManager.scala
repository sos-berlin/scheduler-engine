package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.ScalaUtils._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import java.sql.{Connection, PreparedStatement}
import javax.persistence.EntityManager
import org.hibernate.jdbc.Work
import scala.collection.immutable
import scala.language.implicitConversions
import scala.reflect.ClassTag

class RichEntityManager(val delegate: EntityManager) extends AnyVal {
  def findOption[E](key: AnyRef)(implicit c: ClassTag[E]): Option[E] =
    findOption(key, c.runtimeClass.asInstanceOf[Class[E]])

  def findOption[E](key: AnyRef, clas: Class[E]): Option[E] =
    Option(delegate.find(clas, key))

  def fetchOption[E <: AnyRef : ClassTag](queryString: String, arguments: Iterable[(String, AnyRef)] = Nil): Option[E] =
    fetchOption[E](queryString, implicitClass[E], arguments)

  def fetchOption[E <: AnyRef](queryString: String, clas: Class[E], arguments: Iterable[(String, AnyRef)] = Nil): Option[E] = {
    val i = fetchSeq(queryString, clas, arguments).iterator
    if (i.hasNext) Some(i.next()) ensuring { _ ⇒ !i.hasNext }
    else None
  }

  def fetchSeq[A <: AnyRef : ClassTag](queryString: String, arguments: Iterable[(String, AnyRef)] = Nil): immutable.Seq[A] =
    fetchSeq[A](queryString, implicitClass[A], arguments)

  def fetchSeq[A <: AnyRef](queryString: String, clas: Class[A], arguments: Iterable[(String, AnyRef)] = Nil): immutable.Seq[A] = {
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
      def execute(connection: Connection) {
        result = Some(f(connection))
      }
    })
    result.get
  }
}

object RichEntityManager {
  implicit def toRichEntityManager(e: EntityManager) = new RichEntityManager(e)
}
