// $Id: java_Variant.cxx 11394 2005-04-03 08:30:29Z jz $

#include "z_java.h"

using namespace zschimmer;
using namespace zschimmer::com;

//------------------------------------------------------------------------sos.zschimmer.com.Variant

JNIEXPORT void JNICALL Java_sos_zschimmer_com_Variant_set_1null(JNIEnv *, jobject);

//----------------------------------------------------------------sos_zschimmer_com_Variant.type

JNIEXPORT jint JNICALL Java_sos_zschimmer_com_Variant_is_1null(JNIEnv *, jobject);

/*
 * Class:     sos_zschimmer_com_Variant
 * Method:    assign
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_sos_zschimmer_com_Variant_assign__I
  (JNIEnv *, jobject, jint);

/*
 * Class:     sos_zschimmer_com_Variant
 * Method:    assign
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_sos_zschimmer_com_Variant_assign__Z
  (JNIEnv *, jobject, jboolean);

/*
 * Class:     sos_zschimmer_com_Variant
 * Method:    assign
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_sos_zschimmer_com_Variant_assign__D
  (JNIEnv *, jobject, jdouble);

/*
 * Class:     sos_zschimmer_com_Variant
 * Method:    assign
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_sos_zschimmer_com_Variant_assign__Ljava_lang_String_2
  (JNIEnv *, jobject, jstring);

/*
 * Class:     sos_zschimmer_com_Variant
 * Method:    as_int
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_sos_zschimmer_com_Variant_as_1int
  (JNIEnv *, jobject);

/*
 * Class:     sos_zschimmer_com_Variant
 * Method:    as_boolean
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_sos_zschimmer_com_Variant_as_1boolean
  (JNIEnv *, jobject);

/*
 * Class:     sos_zschimmer_com_Variant
 * Method:    as_double
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_sos_zschimmer_com_Variant_as_1double
  (JNIEnv *, jobject);

/*
 * Class:     sos_zschimmer_com_Variant
 * Method:    as_string
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_sos_zschimmer_com_Variant_as_1string
  (JNIEnv *, jobject);
