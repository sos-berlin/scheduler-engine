package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.scalautil.Logger
import javax.persistence.{EntityManagerFactory, EntityManager}

object ScalaHibernate {

  private val logger = Logger(getClass)

  def transaction[A](f: EntityManager => A)(implicit entityManagerFactory: EntityManagerFactory): A =
    transaction(entityManagerFactory)(f)

  def transaction[A](entityManagerFactory: EntityManagerFactory)(f: EntityManager => A): A =
    autoClosing(entityManagerFactory.createEntityManager()) { entityManager =>
      transaction(entityManager)(f)
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
