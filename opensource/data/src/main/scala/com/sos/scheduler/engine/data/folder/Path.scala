package com.sos.scheduler.engine.data.folder

import com.sos.scheduler.engine.data.base.IsString

trait Path extends IsString {

  final def assertIsAbsolute() {
    if (!isAbsolute) throw new RuntimeException(s"Absolute path expected: $toString")
  }

  final def assertIsEmptyOrAbsolute() {
    val ok = isEmpty || isAbsolute
    if (!ok) throw new RuntimeException(s"Absolute path expected: $toString")
  }

  final def isAbsolute =
    string startsWith "/"
}
