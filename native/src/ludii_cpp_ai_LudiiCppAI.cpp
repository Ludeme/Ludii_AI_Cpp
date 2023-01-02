#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>
#include "ludii_cpp_ai_LudiiCppAI.h"

// NOTE: String descriptions of signatures of Java methods can be found by
// navigating to the directory containing the .class files and using:
//
// javap -s <ClassName>.class

// Java classes we'll need
jclass clsLudiiGameWrapper;
jclass clsLudiiStateWrapper;

// Java method IDs for all the methods we'll need to call
jmethodID midLudiiGameWrapperCtor;
jmethodID midIsStochasticGame;
jmethodID midIsImperfectInformationGame;
jmethodID midIsSimultaneousMoveGame;
jmethodID midNumPlayers;

jmethodID midLudiiStateWrapperCtor;
jmethodID midLudiiStateWrapperCopyCtor;
jmethodID midIsTerminal;

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

/**
 * Perform some static initialisation by caching all the Java classes and
 * Java Method IDs we need.
 */
JNIEXPORT void JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeStaticInit
(JNIEnv* jenv, jclass)
{
	// Classes
	clsLudiiGameWrapper = (jclass) jenv->NewGlobalRef(jenv->FindClass("utils/LudiiGameWrapper"));
	CheckJniException(jenv);

	clsLudiiStateWrapper = (jclass) jenv->NewGlobalRef(jenv->FindClass("utils/LudiiStateWrapper"));
	CheckJniException(jenv);

	// Game wrapper methods
	midLudiiGameWrapperCtor = jenv->GetMethodID(clsLudiiGameWrapper, "<init>", "(Lgame/Game;)V");
	CheckJniException(jenv);

	midIsStochasticGame = jenv->GetMethodID(clsLudiiGameWrapper, "isStochasticGame", "()Z");
	CheckJniException(jenv);

	midIsImperfectInformationGame = jenv->GetMethodID(clsLudiiGameWrapper, "isImperfectInformationGame", "()Z");
	CheckJniException(jenv);

	midIsSimultaneousMoveGame = jenv->GetMethodID(clsLudiiGameWrapper, "isSimultaneousMoveGame", "()Z");
	CheckJniException(jenv);

	midNumPlayers = jenv->GetMethodID(clsLudiiGameWrapper, "numPlayers", "()I");
	CheckJniException(jenv);

	// State wrapper methods
	midLudiiStateWrapperCtor = jenv->GetMethodID(clsLudiiStateWrapper, "<init>", "(Lutils/LudiiGameWrapper;Lother/context/Context;)V");
	CheckJniException(jenv);

	midLudiiStateWrapperCopyCtor = jenv->GetMethodID(clsLudiiStateWrapper, "<init>", "(Lutils/LudiiStateWrapper;)V");
	CheckJniException(jenv);

	midIsTerminal = jenv->GetMethodID(clsLudiiStateWrapper, "isTerminal", "()Z");
	CheckJniException(jenv);
}

/**
 * Node for MCTS
 */
struct MCTSNode {
	MCTSNode(JNIEnv* jenv, const std::shared_ptr<MCTSNode>& parent, jobject wrappedState) :
		parent(parent), visitCount(0) {

		this->wrappedState = jenv->NewObject(clsLudiiStateWrapper, midLudiiStateWrapperCopyCtor, wrappedState);
	}

	~MCTSNode() {
		if (wrappedState != nullptr) {
			std::cerr << "Warning! MCTSNode destructor called with non-null context!" << std::endl;
		}
	}

	void ClearAllJavaRefs(JNIEnv* jenv) {
		jenv->DeleteLocalRef(wrappedState);
		wrappedState = nullptr;

		for (MCTSNode& childNode : childNodes) {
			childNode.ClearAllJavaRefs(jenv);
		}
	}

	std::vector<MCTSNode> childNodes;
	std::vector<double> scoreSums;
	jobject wrappedState;
	std::weak_ptr<MCTSNode> parent;
	uint32_t visitCount;
};

MCTSNode* Select(JNIEnv* jenv, MCTSNode* current) {
	// TODO implement UCB1
	return nullptr;
}

JNIEXPORT jobject JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeSelectAction
(JNIEnv* jenv, jobject jobjAI, jobject game, jobject context, jdouble maxSeconds,
		jint maxIterations, jint maxDepth)
{
	jobject wrappedGame = jenv->NewObject(clsLudiiGameWrapper, midLudiiGameWrapperCtor, game);
	jobject wrappedRootContext = jenv->NewObject(clsLudiiStateWrapper, midLudiiStateWrapperCtor, wrappedGame, context);

	auto root = std::make_shared<MCTSNode>(jenv, std::shared_ptr<MCTSNode>(), wrappedRootContext);
	const int numPlayers = (int)jenv->CallIntMethod(wrappedGame, midNumPlayers);

	// We'll respect time and iteration limits: ignore the maxDepth param
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	double allowedMillis = ((double) maxSeconds) * 1000.0;
	if (allowedMillis <= 0.0)
		allowedMillis = std::numeric_limits<double>::max();

	int iterationsCap = (int) maxIterations;
	if (iterationsCap <= 0)
		iterationsCap = std::numeric_limits<int>::max();

	int numIterations = 0;
	while (numIterations < iterationsCap) {
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		const auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(now - begin).count();
		if (elapsedMillis >= allowedMillis) {
			break;
		}

		// Traverse the tree
		MCTSNode* current = root.get();

		while (true) {
			if (jenv->CallBooleanMethod(current->wrappedState, midIsTerminal)) {
				// Reached a terminal state in the tree
				break;
			}

			current = Select(jenv, current);
		}
	}

	// Clean up local refs to Java objects we created
	root->ClearAllJavaRefs(jenv);

	jenv->DeleteLocalRef(wrappedGame);
	jenv->DeleteLocalRef(wrappedRootContext);
	CheckJniException(jenv);

	return nullptr;
}

JNIEXPORT void JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeInitAI
(JNIEnv* jenv, jobject jobjAI, jobject game, jint playerID)
{
	// Nothing to do here
}

JNIEXPORT void JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeCloseAI
(JNIEnv* jenv, jobject jobjAI)
{
	// Nothing to do here

	// NOTE: could consider deleting the global refs for Java classes here,
	// but then we'd also have to create them again in nativeInitAI(), instead
	// of only in the nativeStaticInit() function
}

JNIEXPORT jboolean JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeSupportsGame
(JNIEnv* jenv, jobject jobjAI, jobject game)
{
	// Wrap the game in a LudiiGameWrapper object
	jobject wrappedGame = jenv->NewObject(clsLudiiGameWrapper, midLudiiGameWrapperCtor, game);

	// Our example UCT does not support stochastic games
	if (jenv->CallBooleanMethod(wrappedGame, midIsStochasticGame))
	{
		jenv->DeleteLocalRef(wrappedGame);
		return false;
	}

	// We also don't support hidden info games
	if (jenv->CallBooleanMethod(wrappedGame, midIsImperfectInformationGame))
	{
		jenv->DeleteLocalRef(wrappedGame);
		return false;
	}

	// And we don't support simultaneous-move games
	if (jenv->CallBooleanMethod(wrappedGame, midIsSimultaneousMoveGame))
	{
		jenv->DeleteLocalRef(wrappedGame);
		return false;
	}

	// Always need to delete our local ref again
	jenv->DeleteLocalRef(wrappedGame);

	// We support anything else
	return true;
}

