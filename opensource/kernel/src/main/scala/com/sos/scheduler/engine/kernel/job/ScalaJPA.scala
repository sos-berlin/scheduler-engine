package com.sos.scheduler.engine.kernel.job

import javax.persistence.{EntityManagerFactory, EntityManager}
import org.slf4j.LoggerFactory
import scala.collection.JavaConversions._

object ScalaJPA {
  private val logger = LoggerFactory.getLogger("com.sos.scheduler.engine.kernel.job.ScalaJPA")

  def transaction[A](f : EntityManager => A)(implicit entityManagerFactory: EntityManagerFactory): A = {
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
          tryRollback(t)
          throw t
      }
    }

    def tryRollback(t: Throwable) {
      try transaction.rollback()
      catch {
        case xx: Exception =>
          logger.error("Second error while rollback ignored: {}", xx)
          logger.error("Original error: {}", t)
      }
    }

    try doTransaction()
    finally entityManager.close()
  }

  implicit def toRichEntityManager(e: EntityManager) = new RichEntityManager(e)

  class RichEntityManager(entityManager: EntityManager) {
    def findOption[E](key: AnyRef)(implicit m: Manifest[E]): Option[E] =
      Option(entityManager.find(m.erasure, key).asInstanceOf[E])

    def fetchSeq[A <: AnyRef](queryString: String, arguments: Iterable[(String, AnyRef)] = Iterable())
        (implicit m: Manifest[A]): Seq[A] = {
      val q = entityManager.createQuery(queryString, m.erasure)
      for ((name, value) <- arguments) q.setParameter(name, value)
      q.getResultList.asInstanceOf[java.util.List[A]].toSeq
    }
  }
}
