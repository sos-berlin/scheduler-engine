package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.kernel.job.ScalaJPA._
import javax.persistence.EntityManagerFactory

/** JPA-Operationen mit Konvertierung zwischen Entity und Objekt.
  * @tparam E Entity-Klasse, mit @Entity annotiert
  * @tparam OBJ Objektklasse
  * @tparam KEY Schlüsselklasse des Objekts */
abstract class AbstractObjectJPAStore[E <: AnyRef, OBJ <: AnyRef, KEY](
  implicit entityManifest: Manifest[E]) {

  implicit protected val entityManagerFactory: EntityManagerFactory

  protected def toEntityKey(key: KEY): AnyRef

  protected def toEntity(o: OBJ): E

  protected def toObject(e: E): OBJ

  def tryFetch(key: KEY) =
    transaction { entityManager =>
      entityManager.findOption[E](toEntityKey(key))(entityManifest) map toObject
    }

  def store(o: OBJ) {
      val e = toEntity(o)
      transaction { entityManager =>
        entityManager.merge(e)
      }
  }

  def delete(key: KEY) {
    transaction { entityManager =>
      entityManager.findOption[E](toEntityKey(key))(entityManifest) foreach { e =>
        entityManager.remove(e)
      }
    }
  }
}

object AbstractObjectJPAStore {
  trait HasKey[K] {
    val key: K
  }
}
