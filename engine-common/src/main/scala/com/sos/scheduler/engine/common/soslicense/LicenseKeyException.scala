package com.sos.scheduler.engine.common.soslicense

import com.sos.scheduler.engine.base.exceptions.StandardPublicException
import com.sos.scheduler.engine.common.soslicense.LicenseKey.Parameter
import com.sos.scheduler.engine.common.soslicense.Parameters.parameterToString

/**
 * @author Joacim Zschimmer
 */
sealed abstract class LicenseKeyException(message: String) extends StandardPublicException(message)

final class LicenseKeyParameterExpiredException(parameter: Parameter, failureText: String = "")
extends LicenseKeyException(s"License key expired for '${parameterToString(parameter)}'" +
  (if (failureText.isEmpty) "" else s" ($failureText)"))

final class LicenseKeyParameterIsMissingException(parameter: Parameter, failureText: String = "")
extends LicenseKeyException(s"LicenseKeyParameterIsMissingException: License key required for '${parameterToString(parameter)}'" +
  (if (failureText.isEmpty) "" else s" ($failureText)"))

final class LicenseKeyRequiredException extends LicenseKeyException("License key is required")
