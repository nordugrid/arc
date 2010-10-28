#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef __JNI_INCLUDE_ARC__ 
#define __JNI_INCLUDE_ARC__ 
#ifdef HAVE_JNI_H
#include <jni.h>
#else
#ifdef HAVE_JAVAVM_JNI_H
#include <JavaVM/jni.h>
#endif
#endif
#endif
