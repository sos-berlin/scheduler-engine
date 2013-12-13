package com.sos.scheduler.engine.data.folder

import com.sos.scheduler.engine.data.base.IsString

trait Path extends IsString {

  final def assertIsEmptyOrAbsolute() {
    if (!isEmpty)
      assertIsAbsolute()
  }

  final def assertIsAbsolute() {
    require(isAbsolute, s"Absolute path expected: $toString")
  }

  final def isAbsolute =
    string startsWith "/"
}
