package com.sos.scheduler.engine.cplusplus.generator.module


/** Dummy-Modul für Javadoc, damit es nicht mangels Klasse mit öffentlichem Konstruktor abbricht.
 *  (Die Java-Proxies werden über JNI angelegt.)
*/

class JavadocJavaModule(packageName: String) extends JavaModule {
    private val simpleName = "ZZZJavadocDummy"
    val name = packageName + "." + simpleName

    val code = "package " + packageName + ";\n" +
        "/** Dummy-Modul für Javadoc, damit es nicht mangels Klasse mit öffentlichem Konstruktor abbricht. */\n\n" +
        "public final class " + simpleName + "{\n" +
        "    public " + simpleName + "() { throw new UnsupportedOperationException(\"" + name + "\"); }\n" +
        "}\n";
}
