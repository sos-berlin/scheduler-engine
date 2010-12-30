package com.sos.scheduler.kernel.cplusplus.generator.module

import com.sos.scheduler.kernel.cplusplus.generator.Configuration._
import com.sos.scheduler.kernel.cplusplus.generator.util.Util._


class MakefileInclude(prefix: String, modules: Iterable[Module])
extends Module {
    val name = prefix + ".makefile.include"

    val codeFiles = List(new CodeFile {
        val path = name
        val encoding = unixEncoding

        val content =
            "# " + commentLine + "\n" +
            prefix + "_objects=\\\n" + {
                val objectNames = (modules map { _.name + ".o" }).toSeq.sorted
                objectNames.mkString(" ", "\\\n ", "\n")
            }
    })
}
