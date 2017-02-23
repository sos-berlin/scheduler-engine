package com.sos.scheduler.engine.kernel.messagecode

import com.sos.jobscheduler.data.message.MessageCode
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class MessageCodeHandlerTest extends FreeSpec {

  private val messageCodeHandler = MessageCodeHandler.fromCodeAndTextStrings(List(
    "TEST-0 test-0",
    "TEST-1 one=$1",
    "TEST-2 one=$1 two=$2",
    "TEST-31 three=$3 dollar=$$ dollar=$ one=$1"))

  "No insertion" in {
    assert(messageCodeHandler(MessageCode("TEST-0")) == "TEST-0 test-0")
    assert(messageCodeHandler(MessageCode("TEST-1")) == "TEST-1 one=$1")
    assert(messageCodeHandler(MessageCode("TEST-2")) == "TEST-2 one=$1 two=$2")
    assert(messageCodeHandler(MessageCode("TEST-31")) == "TEST-31 three=$3 dollar=$ dollar=$ one=$1")
    assert(messageCodeHandler(MessageCode("TEST-UNKNOWN")) == "TEST-UNKNOWN")
  }

  "One insertion" in {
    assert(messageCodeHandler(MessageCode("TEST-0"), "ONE") == "TEST-0 test-0 [ONE]")
    assert(messageCodeHandler(MessageCode("TEST-1"), "ONE") == "TEST-1 one=ONE")
    assert(messageCodeHandler(MessageCode("TEST-2"), "ONE") == "TEST-2 one=ONE two=$2")
    assert(messageCodeHandler(MessageCode("TEST-31"), "ONE") == "TEST-31 three=$3 dollar=$ dollar=$ one=ONE")
    assert(messageCodeHandler(MessageCode("TEST-UNKNOWN"), "ONE") == "TEST-UNKNOWN [ONE]")
  }

  "Two insertions" in {
    assert(messageCodeHandler(MessageCode("TEST-0"), "ONE", "TWO") == "TEST-0 test-0 [ONE] [TWO]")
    assert(messageCodeHandler(MessageCode("TEST-1"), "ONE", "TWO") == "TEST-1 one=ONE [TWO]")
    assert(messageCodeHandler(MessageCode("TEST-2"), "ONE", "TWO") == "TEST-2 one=ONE two=TWO")
    assert(messageCodeHandler(MessageCode("TEST-31"), "ONE", "TWO") == "TEST-31 three=$3 dollar=$ dollar=$ one=ONE [TWO]")
    assert(messageCodeHandler(MessageCode("TEST-UNKNOWN"), "ONE", "TWO") == "TEST-UNKNOWN [ONE] [TWO]")
  }

  "Three insertions" in {
    assert(messageCodeHandler(MessageCode("TEST-0"), "ONE", "TWO", "THREE") == "TEST-0 test-0 [ONE] [TWO] [THREE]")
    assert(messageCodeHandler(MessageCode("TEST-1"), "ONE", "TWO", "THREE") == "TEST-1 one=ONE [TWO] [THREE]")
    assert(messageCodeHandler(MessageCode("TEST-2"), "ONE", "TWO", "THREE") == "TEST-2 one=ONE two=TWO [THREE]")
    assert(messageCodeHandler(MessageCode("TEST-31"), "ONE", "TWO", "THREE") == "TEST-31 three=THREE dollar=$ dollar=$ one=ONE [TWO]")
    assert(messageCodeHandler(MessageCode("TEST-UNKNOWN"), "ONE", "TWO", "THREE") == "TEST-UNKNOWN [ONE] [TWO] [THREE]")
  }

  "Long insertion is clipped" in {
    val longInserion = "Long " * 1000
    val clipped = longInserion.substring(0, MessageCodeHandler.InsertionLengthMaximum)
    assert(messageCodeHandler(MessageCode("TEST-1"), longInserion, longInserion) == s"TEST-1 one=$clipped [$clipped]")
  }
}
