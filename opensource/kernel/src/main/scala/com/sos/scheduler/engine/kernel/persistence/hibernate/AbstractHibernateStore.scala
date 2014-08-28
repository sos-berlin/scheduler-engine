package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.data.base.{HasKey, HasIsDefault}
import com.sos.scheduler.engine.kernel.persistence.hibernate.RichEntityManager.toRichEntityManager
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter
import javax.persistence.EntityManager

/** JPA-Operationen mit Konvertierung zwischen Entity und Objekt.
  * @tparam E Entity-Klasse, mit @Entity annotiert
  * @tparam OBJ Objektklasse
  * @tparam KEY Schlüsselklasse des Objekts */
abstract class AbstractHibernateStore[OBJ <: HasKey[KEY], KEY, E <: AnyRef](implicit entityManifest: Manifest[E])
extends ObjectEntityConverter[OBJ, KEY, E]
with JPAStore[OBJ, KEY] {

  def tryFetch(key: KEY)(implicit em: EntityManager) =
    em.findOption[E](toEntityKey(key))(entityManifest) map toObject

  def insert(o: OBJ)(implicit em: EntityManager): Unit = {
    em.persist(toEntity(o))
  }

  def store(o: OBJ)(implicit em: EntityManager): Unit = {
    o match {
      case d: HasIsDefault if d.isDefault =>
        delete(o.key)  // Den Default-Zustand speichern wir nicht
      case _ =>
        em.merge(toEntity(o))
    }
  }

  def delete(key: KEY)(implicit em: EntityManager): Unit = {
    em.findOption[E](toEntityKey(key))(entityManifest) foreach { e =>
      em.remove(e)
    }
  }
}
