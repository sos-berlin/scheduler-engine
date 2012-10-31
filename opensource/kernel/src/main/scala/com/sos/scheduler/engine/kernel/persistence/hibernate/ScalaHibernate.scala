package com.sos.scheduler.engine.kernel.persistence.hibernate

import javax.persistence.EntityManager
import org.slf4j.LoggerFactory

object ScalaHibernate {
  private val logger = LoggerFactory.getLogger("com.sos.scheduler.engine.kernel.job.ScalaHibernate")

  def transaction[A](f: EntityManager => A)(implicit entityManager: EntityManager): A =
    transaction(entityManager)(f)

//  def transaction[A](f: EntityManager => A)(implicit entityManagerFactory: EntityManagerFactory): A = {
//    val entityManager = entityManagerFactory.createEntityManager()
//    val result = transaction(entityManager)(f)
//    entityManager.close()
//    result
//  }

  def transaction[A](entityManager: EntityManager)(f: EntityManager => A): A = {
    val ta = entityManager.getTransaction

    def tryRollback(t: Throwable) {
      try ta.rollback()
      catch {
        case x: Exception => logger.error("Second error in rollback ignored:", x)
      }
    }

    ta.begin()
    try {
      val result = f(entityManager)
      ta.commit()
      result
    } catch {
      case t: Throwable if ta.isActive =>
        tryRollback(t)
        throw t
    }
  }
}
