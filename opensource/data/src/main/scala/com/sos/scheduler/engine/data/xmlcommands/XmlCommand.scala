package com.sos.scheduler.engine.data.xmlcommands

trait XmlCommand {
  def xmlElem: scala.xml.Elem
  def xmlString: String = xmlElem.toString()
}
