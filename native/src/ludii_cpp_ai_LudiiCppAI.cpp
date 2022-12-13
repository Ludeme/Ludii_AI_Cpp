#include <iostream>
#include <stdexcept>
#include "ludii_cpp_ai_LudiiCppAI.h"

namespace {
	
	// Java classes we'll need
	jclass clsLudiiGameWrapper;
	
	// Java method IDs for all the methods we'll need to call
	jmethodID midLudiiGameWrapperCtor;
	
	/**
	 * It is good practice to call this after basically any call to a Java method
	 * that might throw an exception.
	 */
	static void CheckJniException(JNIEnv* jenv) {
		if (jenv->ExceptionCheck()) {
		  jenv->ExceptionDescribe();
		  jenv->ExceptionClear();
		  printf("Java Exception at line %d of %s\n", __LINE__, __FILE__);
		  throw std::runtime_error("Java exception thrown!");
		}
	}

	JNIEXPORT void JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeStaticInit
	  (JNIEnv* jenv, jclass)
	  {
		  midLudiiGameWrapperCtor = jenv->GetMethodID();
	  }

	JNIEXPORT jobject JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeSelectAction
		(JNIEnv* jenv, jobject jobjAI, jobject game, jobject context, jdouble maxSeconds, 
		jint maxIterations, jint maxDepth)
		{
			std::cout << "native implementation for selectAction()!" << std::endl;
			return nullptr;
		}


	JNIEXPORT void JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeInitAI
	  (JNIEnv* jenv, jobject jobjAI, jobject game, jint playerID)
	  {
		  std::cout << "native implementation for initAI()!" << std::endl;
	  }


	JNIEXPORT void JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeCloseAI
	  (JNIEnv* jenv, jobject jobjAI)
	  {
		  std::cout << "native implementation for closeAI()!" << std::endl;
	  }
	  
	JNIEXPORT jboolean JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeSupportsGame
	  (JNIEnv* jenv, jobject jobjAI, jobject game)
	  {
		  // Our example UCT does not support stochastic games
		  
		  std::cout << "native implementation for supportsGame()!" << std::endl;
		  
		  // We support anything else
		  return true;
	  }
}

