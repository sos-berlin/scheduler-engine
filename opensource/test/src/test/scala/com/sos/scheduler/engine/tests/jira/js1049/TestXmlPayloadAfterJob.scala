package com.sos.scheduler.engine.tests.jira.js1049

final class TestXmlPayloadAfterJob extends sos.spooler.Job_impl {
  override def spooler_process() = {
    //Nur XML-Wurzelelement wird übernommen - require(spooler_task.order.xml_payload contains "<?xml-stylesheet")
    require(spooler_task.order.xml_payload contains "<settings")
    true
  }
}
