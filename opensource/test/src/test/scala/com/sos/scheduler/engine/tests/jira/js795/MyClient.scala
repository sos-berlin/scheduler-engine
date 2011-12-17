package com.sos.scheduler.engine.tests.jira.js795

import java.net.URL
import com.google.common.base.Charsets.UTF_8
import org.eclipse.jetty.client.{ContentExchange, HttpExchange, HttpClient}
import org.eclipse.jetty.io.ByteArrayBuffer
import org.scalatest.Assertions._

@deprecated("","")
private class MyClient(url: URL) {
    private val encoding = UTF_8
    val httpClient = new HttpClient
    httpClient.setConnectorType(HttpClient.CONNECTOR_SELECT_CHANNEL)
    httpClient.start()

    def executeXml(commandXml: String): String = {
        val c = newContentExchange()
        c.setRequestContent(new ByteArrayBuffer(commandXml, encoding.name))
        c.setMethod("POST")
        httpClient.send(c)
        val sendStatus = c.waitForDone()
        assert(sendStatus === HttpExchange.STATUS_COMPLETED)
        assert(c.getResponseStatus === 200)
        c.getResponseContent
    }

    private def newContentExchange() = {
        val result = new ContentExchange
        result.setMethod("POST")
        result.setURL(url.toExternalForm)
        result.setRequestContentType("text/xml;charset="+ encoding.name)
        result
    }
}
