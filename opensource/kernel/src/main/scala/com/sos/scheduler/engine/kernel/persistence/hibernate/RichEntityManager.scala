package com.sos.scheduler.engine.kernel.persistence.hibernate

import java.sql.{Connection, PreparedStatement}
import javax.persistence.EntityManager
import org.hibernate.jdbc.Work
import scala.collection.JavaConversions.collectionAsScalaIterable
import scala.collection.immutable
import scala.reflect.ClassTag

class RichEntityManager(entityManager: EntityManager) {
  def findOption[E](key: AnyRef)(implicit c: ClassTag[E]): Option[E] =
    findOption(key, c.runtimeClass.asInstanceOf[Class[E]])

  def findOption[E](key: AnyRef, clas: Class[E]): Option[E] =
    Option(entityManager.find(clas, key))

  def fetchOption[E <: AnyRef](queryString: String, arguments: Iterable[(String, AnyRef)] = Iterable())(implicit c: ClassTag[E]): Option[E] =
    fetchOption[E](queryString, c.runtimeClass.asInstanceOf[Class[E]], arguments)

  def fetchOption[E <: AnyRef](queryString: String, clas: Class[E], arguments: Iterable[(String, AnyRef)] = Iterable()): Option[E] = {
    val i = fetchSeq(queryString, clas, arguments).iterator
    if (i.hasNext) Some(i.next()) ensuring { _ => !i.hasNext }
    else None
  }

  def fetchSeq[A <: AnyRef](queryString: String, arguments: Iterable[(String, AnyRef)] = Iterable())(implicit c: ClassTag[A]): immutable.Seq[A] =
    fetchSeq[A](queryString, c.runtimeClass.asInstanceOf[Class[A]], arguments)

  def fetchSeq[A <: AnyRef](queryString: String, clas: Class[A], arguments: Iterable[(String, AnyRef)] = Iterable()): immutable.Seq[A] = {
    val q = entityManager.createQuery(queryString, clas)
    for ((name, value) <- arguments) q.setParameter(name, value)
    immutable.Seq() ++ q.getResultList
  }

  def useJDBCPreparedStatement[A](sql: String)(f: PreparedStatement => A) = {
    useJDBCConnection { connection =>
      val preparedStatement = connection.prepareStatement(sql)
      try f(preparedStatement)
      finally preparedStatement.close()
    }
  }

  def useJDBCConnection[A](f: Connection => A): A = {
    // Geht nicht mit Hibernate 4.1.7 (aber mit EclipseLink): f(em.unwrap(classOf[java.sql.Connection]))
    var result: Option[A] = None
    entityManager.unwrap(classOf[org.hibernate.Session]).doWork(new Work {
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