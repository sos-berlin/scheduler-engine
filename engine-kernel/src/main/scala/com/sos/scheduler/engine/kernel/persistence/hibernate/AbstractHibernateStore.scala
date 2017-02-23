package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.jobscheduler.base.generic.HasIsDefault
import com.sos.jobscheduler.base.utils.HasKey
import com.sos.scheduler.engine.kernel.persistence.hibernate.RichEntityManager.toRichEntityManager
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter
import javax.persistence.EntityManager

/** JPA-Operationen mit Konvertierung zwischen Entity und Objekt.
 *
  * @tparam E Entity-Klasse, mit @Entity annotiert
  * @tparam A Objektklasse
  */
abstract class AbstractHibernateStore[A <: HasKey, E <: AnyRef](implicit entityManifest: Manifest[E])
extends ObjectEntityConverter[A, A#Key, E]
with JPAStore[A] {

  def tryFetch(key: A#Key)(implicit em: EntityManager) =
    em.findOption[E](toEntityKey(key))(entityManifest) map toObject

  def insert(o: A)(implicit em: EntityManager): Unit = {
    em.persist(toEntity(o))
  }

  def store(o: A)(implicit em: EntityManager): Unit = {
    o match {
      case d: HasIsDefault if d.isDefault =>
        delete(o.key)  // Den Default-Zustand speichern wir nicht
      case _ =>
        em.merge(toEntity(o))
    }
  }

  def delete(key: A#Key)(implicit em: EntityManager): Unit = {
    em.findOption[E](toEntityKey(key))(entityManifest) foreach { e =>
      em.remove(e)
    }
  }
}
