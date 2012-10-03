package com.sos.scheduler.engine.kernel.persistence

import com.sos.scheduler.engine.kernel.persistence.ScalaJPA._
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter
import javax.persistence.EntityManager

/** JPA-Operationen mit Konvertierung zwischen Entity und Objekt.
  * @tparam E Entity-Klasse, mit @Entity annotiert
  * @tparam OBJ Objektklasse
  * @tparam KEY Schlüsselklasse des Objekts */
abstract class AbstractObjectJPAStore[OBJ <: AnyRef, KEY, E <: AnyRef](implicit entityManifest: Manifest[E])
extends ObjectEntityConverter[OBJ, KEY, E] {

  def tryFetch(key: KEY)(implicit em: EntityManager) =
    em.findOption[E](toEntityKey(key))(entityManifest) map toObject

  def insert(o: OBJ)(implicit em: EntityManager) {
    em.persist(toEntity(o))
  }

  def store(o: OBJ)(implicit em: EntityManager) {
    em.merge(toEntity(o))
  }

  def delete(key: KEY)(implicit em: EntityManager) {
    em.findOption[E](toEntityKey(key))(entityManifest) foreach { e =>
      em.remove(e)
    }
  }
}
