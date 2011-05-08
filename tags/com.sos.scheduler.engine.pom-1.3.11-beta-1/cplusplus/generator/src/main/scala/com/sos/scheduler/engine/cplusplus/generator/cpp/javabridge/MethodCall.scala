package com.sos.scheduler.engine.cplusplus.generator.cpp.javabridge

import com.sos.scheduler.engine.cplusplus.generator.util.ClassOps._


class MethodCall private(val returnType: String, val callMethod: String)


object MethodCall {    
    /** Methoden der C++-Klasse javabridge::Method */
    private val typedCalls = Map[Class[_],MethodCall] (
        classOf[Void   ] -> new MethodCall("void"   , "call"        ),
        classOf[Boolean] -> new MethodCall("bool"   , "bool_call"   ),
        classOf[Char   ] -> new MethodCall("wchar_t", "char_call"   ),
        classOf[Byte   ] -> new MethodCall("jbyte"  , "byte_call"   ),
        classOf[Short  ] -> new MethodCall("jshort" , "short_call"  ),
        classOf[Int    ] -> new MethodCall("jint"   , "int_call"    ),
        classOf[Long   ] -> new MethodCall("jlong"  , "long_call"   ),
        classOf[Float  ] -> new MethodCall("jfloat" , "float_call"  ),
        classOf[Double ] -> new MethodCall("jdouble", "double_call" ),
        classOf[Object ] -> new MethodCall("jobject", "jobject_call")
    )

    def apply(t: Class[_]) = typedCalls(
        if (isVoid(t)) classOf[Void]
        else if (isClass(t))  classOf[Object]
        else t)
}