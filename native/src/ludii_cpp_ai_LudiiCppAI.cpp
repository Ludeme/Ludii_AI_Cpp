#include <iostream>
#include "ludii_cpp_ai_LudiiCppAI.h"

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
	  std::cout << "native implementation for supportsGame()!" << std::endl;
	  return true;
  }
