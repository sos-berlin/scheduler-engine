package com.sos.scheduler.engine.common.scalautil

import com.google.common.base.Charsets._
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import java.io.File

object ScalaXmls {
  object implicits {
    implicit class RichFile(val delegate: File) extends AnyVal {

      def xml = SafeXML.loadFile(delegate)

      def xml_=(o: scala.xml.Elem) {
        scala.xml.XML.save(delegate.getPath, o, enc = UTF_8.name, xmlDecl = true)
      }
    }
  }
}
