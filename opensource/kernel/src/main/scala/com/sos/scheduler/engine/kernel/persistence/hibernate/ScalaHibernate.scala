package com.sos.scheduler.engine.kernel.persistence.hibernate

import java.sql.{PreparedStatement, Connection}
import javax.persistence.{EntityManagerFactory, EntityManager}
import org.hibernate.jdbc.Work
import org.slf4j.LoggerFactory
import scala.Some
import scala.collection.JavaConversions._

object ScalaHibernate {
  private val logger = LoggerFactory.getLogger("com.sos.scheduler.engine.kernel.job.ScalaHibernate")

  def transaction[A](f: EntityManager => A)(implicit entityManagerFactory: EntityManagerFactory): A =
    transaction(entityManagerFactory)(f)

  def transaction[A](entityManagerFactory: EntityManagerFactory)(f: EntityManager => A): A = {
    val entityManager = entityManagerFactory.createEntityManager()
    val transaction = entityManager.getTransaction

    def doTransaction() = {
      transaction.begin()
      try {
        val result = f(entityManager)
        transaction.commit()
        result
      } catch {
        case t: Throwable =>
          if (transaction.isActive) tryRollback(t)
          throw t
      }
    }

    def tryRollback(t: Throwable) {
      try transaction.rollback()
      catch {
        case xx: Exception =>
          logger.error("Second error while rollback ignored:", xx)
          logger.error("Original error:", t)
      }
    }

    try doTransaction()
    finally entityManager.close()
  }

  def useJDBCPreparedStatement[A](sql: String)(f: PreparedStatement => A)(implicit em: EntityManager) = {
    useJDBCConnection { connection =>
      val preparedStatement = connection.prepareStatement(sql)
      try f(preparedStatement)
      finally preparedStatement.close()
    }
  }

  def useJDBCConnection[A](f: Connection => A)(implicit em: EntityManager): A = {
    // Geht nicht mit Hibernate 4.1.7 (aber mit EclipseLink): f(em.unwrap(classOf[java.sql.Connection]))
    var result: Option[A] = None
    em.unwrap(classOf[org.hibernate.Session]).doWork(new Work {   // Hibernate 4
      def execute(connection: Connection) {
        result = Some(f(connection))
      }
    })
    result.get
  }

  implicit def toRichEntityManager(e: EntityManager) = new RichEntityManager(e)

  class RichEntityManager(entityManager: EntityManager) {
    def findOption[E](key: AnyRef)(implicit m: Manifest[E]): Option[E] =
      Option(entityManager.find(m.erasure, key).asInstanceOf[E])

    def fetchSeq[A <: AnyRef](queryString: String, arguments: Iterable[(String, AnyRef)] = Iterable())(implicit m: Manifest[A]): Seq[A] = {
      val q = entityManager.createQuery(queryString, m.erasure)
      for ((name, value) <- arguments) q.setParameter(name, value)
      q.getResultList.asInstanceOf[java.util.List[A]].toSeq
    }

    def fetchOption[A <: AnyRef](queryString: String, arguments: Iterable[(String, AnyRef)] = Iterable())(implicit m: Manifest[A]): Option[A] = {
      val i = fetchSeq(queryString, arguments)(m).iterator
      if (i.hasNext) Some(i.next()) ensuring { _ => !i.hasNext }
      else None
    }
  }
}
