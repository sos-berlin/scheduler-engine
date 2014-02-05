package com.sos.scheduler.engine.tests.jira.js1049

final class TestXmlPayloadJob extends sos.spooler.Job_impl {
  override def spooler_process() = {
    // encoding='ISO-8859-1' ist falsch, aber so ist die Testvorgabe. Offenbar wird das ignoriert.
    spooler_task.order.set_xml_payload("<?xml version='1.0' encoding='ISO-8859-1'?><?xml-stylesheet type='text/xsl' href='scheduler_configuration_documentation.xsl'?> <settings></settings>")
    true
  }
}
