package com.sos.scheduler.engine.minicom.types

import com.sos.scheduler.engine.common.scalautil.ScalaUtils.{cast, implicitClass}
import scala.collection.immutable
import scala.reflect.ClassTag
import scala.runtime.BoxedUnit

/**
 * @author Joacim Zschimmer
 */
object Variant {
  val VT_EMPTY            = 0
  val VT_NULL             = 1
  val VT_I2               = 2
  val VT_I4               = 3
  val VT_R4               = 4
  val VT_R8               = 5
  val VT_CY               = 6
  val VT_DATE             = 7
  val VT_BSTR             = 8
  val VT_DISPATCH         = 9
  val VT_ERROR            = 10
  val VT_BOOL             = 11
  val VT_VARIANT          = 12
  val VT_UNKNOWN          = 13
  val VT_DECIMAL          = 14
  val VT_I1               = 16
  val VT_UI1              = 17
  val VT_UI2              = 18
  val VT_UI4              = 19
  val VT_I8               = 20
  val VT_UI8              = 21
  val VT_INT              = 22
  val VT_UINT             = 23
  val VT_SAFEARRAY        = 27
  val VT_ARRAY            = 0x2000

  val BoxedEmpty = BoxedUnit.UNIT
}

final case class VariantArray(indexedSeq: immutable.IndexedSeq[Any]) {
  def asIUnknowns: immutable.IndexedSeq[IUnknown] =
    indexedSeq.asInstanceOf[immutable.IndexedSeq[Some[_]]] map { case Some(o) ⇒ cast[IUnknown](o) }

  def as[A : ClassTag] = {
    require(indexedSeq forall { _.getClass isAssignableFrom implicitClass[A] })
    indexedSeq.asInstanceOf[immutable.IndexedSeq[A]]
  }
}

object VariantArray {
  final val FADF_FIXEDSIZE   =  0x10
  final val FADF_HAVEVARTYPE =  0x80
  final val FADF_BSTR        = 0x100
  final val FADF_VARIANT     = 0x800
}
