package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.kernel.job.ScalaJPA._
import javax.persistence.EntityManager

/** JPA-Operationen mit Konvertierung zwischen Entity und Objekt.
  * @tparam E Entity-Klasse, mit @Entity annotiert
  * @tparam OBJ Objektklasse
  * @tparam KEY Schlüsselklasse des Objekts */
abstract class AbstractObjectJPAStore[E <: AnyRef, OBJ <: AnyRef, KEY](implicit entityManifest: Manifest[E]) {

  protected def toEntityKey(key: KEY): AnyRef

  protected def toEntity(o: OBJ): E

  protected def toObject(e: E): OBJ

  def tryFetch(key: KEY)(implicit em: EntityManager) =
    em.findOption[E](toEntityKey(key))(entityManifest) map toObject

  def store(o: OBJ)(implicit em: EntityManager) {
    em.merge(toEntity(o))
  }

  def delete(key: KEY)(implicit em: EntityManager) {
    em.findOption[E](toEntityKey(key))(entityManifest) foreach { e =>
      em.remove(e)
    }
  }
}
