package com.sos.scheduler.kernel.cplusplus.runtime.annotation;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/** Für ein Super-Interface eines mit @CppClass annotierten Interfaces:
 * Die Methoden dieses Interfaces sind in Java statt in C++ implementiert.
 */
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.TYPE)
public @interface JavaOnlyInterface {}
