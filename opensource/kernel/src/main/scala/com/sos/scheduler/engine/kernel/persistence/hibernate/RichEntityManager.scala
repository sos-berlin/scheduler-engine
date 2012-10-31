package com.sos.scheduler.engine.kernel.persistence.hibernate

import java.sql.{Connection, PreparedStatement}
import javax.persistence.EntityManager
import org.hibernate.jdbc.Work
import scala.collection.JavaConversions.collectionAsScalaIterable
import scala.collection.immutable

class RichEntityManager(entityManager: EntityManager) {
  def findOption[E](key: AnyRef)(implicit m: Manifest[E]): Option[E] =
    Option(entityManager.find(m.erasure, key).asInstanceOf[E])

  def fetchSeq[A <: AnyRef](queryString: String, arguments: Iterable[(String, AnyRef)] = Iterable())(implicit m: Manifest[A]): immutable.Seq[A] = {
    val q = entityManager.createQuery(queryString, m.erasure)
    for ((name, value) <- arguments) q.setParameter(name, value)
    immutable.Seq() ++ q.getResultList.asInstanceOf[java.util.List[A]]
  }

  def fetchOption[A <: AnyRef](queryString: String, arguments: Iterable[(String, AnyRef)] = Iterable())(implicit m: Manifest[A]): Option[A] = {
    val i = fetchSeq(queryString, arguments)(m).iterator
    if (i.hasNext) Some(i.next()) ensuring { _ => !i.hasNext }
    else None
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