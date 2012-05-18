package com.sos.scheduler.engine.cplusplus.generator.cpp

/** C++-Routinen */
object Cpp {
  private val cppReservedWordsString =
    "bool char double float int long short signed unsigned void wchar_t " +
    "false true " +
    "asm auto break case catch class const const_cast continue default delete do dynamic_cast " +
    "else enum explicit export extern for friend goto if inline mutable namespace new operator " +
    "private protected public register reinterpret_cast return sizeof static static_cast struct switch " +
    "template this throw try typedef typeid typename union using virtual volatile while "
    //"and and_eq bitand bitor not not_eq or or_eq xor xor_eq "

  private val cppReservedWords = (cppReservedWordsString split " +").toSet

  def quoted(s: String) = '"' + s.replace("\"", "\\\"") + '"'

  def includeQuoted(path: String) = "#include " + quoted(path) + "\n"

  def headerOnce(preprocessorMacro: String)(content: String) =
    "#ifndef " + preprocessorMacro + "\n" +
    "#define " + preprocessorMacro + "\n\n" +
    content + "\n" +
    "#endif\n"

  def normalizedName(name: String) = if (cppReservedWords contains name)  name + "_"  else name
}
