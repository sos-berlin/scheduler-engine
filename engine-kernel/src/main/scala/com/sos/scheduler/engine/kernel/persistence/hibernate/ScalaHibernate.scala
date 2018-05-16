package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.common.scalautil.Logger
import javax.persistence.{EntityManager, EntityManagerFactory}

object ScalaHibernate {

  private val logger = Logger(getClass)

  def transaction[A](f: EntityManager => A)(implicit entityManagerFactory: EntityManagerFactory): A =
    transaction(entityManagerFactory)(f)

  def transaction[A](entityManagerFactory: EntityManagerFactory)(f: EntityManager => A): A = {
    val entityManager = entityManagerFactory.createEntityManager()
    try transaction(entityManager)(f)
    finally entityManager.close()
  }

  private def transaction[A](entityManager: EntityManager)(f: EntityManager => A): A = {
    val ta = entityManager.getTransaction

    def tryRollback(t: Throwable): Unit = {
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
