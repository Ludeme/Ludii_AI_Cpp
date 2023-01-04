#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
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
jmethodID midLegalMovesArray;
jmethodID midApplyMove;
jmethodID midCurrentPlayer;

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

	midLegalMovesArray = jenv->GetMethodID(clsLudiiStateWrapper, "legalMovesArray", "()[Lother/move/Move;");
	CheckJniException(jenv);

	midApplyMove = jenv->GetMethodID(clsLudiiStateWrapper, "applyMove", "(Lother/move/Move;)V");
	CheckJniException(jenv);

	midCurrentPlayer = jenv->GetMethodID(clsLudiiStateWrapper, "currentPlayer", "()I");
	CheckJniException(jenv);
}

/**
 * Node for MCTS
 */
struct MCTSNode {
	MCTSNode(JNIEnv* jenv, MCTSNode* parent, jobject wrappedState, const int32_t numPlayers, std::mt19937& rng) :
		parent(parent), visitCount(0) {

		this->wrappedState = jenv->NewGlobalRef(wrappedState);

		const jobjectArray javaMovesArray =
		      static_cast<jobjectArray>(jenv->CallObjectMethod(this->wrappedState, midLegalMovesArray));
		CheckJniException(jenv);
		const jsize numLegalMoves = jenv->GetArrayLength(javaMovesArray);

		for (jsize i = 0; i < numLegalMoves; ++i) {
			unexpandedMoves.push_back(jenv->NewGlobalRef(jenv->GetObjectArrayElement(javaMovesArray, i)));
		}
		std::shuffle(std::begin(unexpandedMoves), std::end(unexpandedMoves), rng);

		std::fill(scoreSums.begin(), scoreSums.begin() + numPlayers, 0.0);

		jenv->DeleteLocalRef(javaMovesArray);
	}

	~MCTSNode() {
		if (wrappedState != nullptr) {
			std::cerr << "Warning! MCTSNode destructor called with non-null context!" << std::endl;
		}
	}

	void ClearAllJavaRefs(JNIEnv* jenv) {
		jenv->DeleteGlobalRef(wrappedState);
		wrappedState = nullptr;

		for (MCTSNode& childNode : childNodes) {
			childNode.ClearAllJavaRefs(jenv);
		}

		for (size_t i = 0; i < unexpandedMoves.size(); ++i) {
			jenv->DeleteGlobalRef(unexpandedMoves[i]);
		}
	}

	std::vector<jobject> unexpandedMoves;
	std::vector<MCTSNode> childNodes;
	std::vector<double> scoreSums;
	jobject wrappedState;
	MCTSNode* parent;
	uint32_t visitCount;
};

MCTSNode* Select(JNIEnv* jenv, MCTSNode* current, std::mt19937& rng) {
	if (!current->unexpandedMoves.empty()) {
		// Randomly select an unexpanded move. The vector was
		// already shuffled after creation, so just pop the last
		// element.
		jobject unexpandedMove = current->unexpandedMoves.back();
		current->unexpandedMoves.pop_back();

		// Create copy of our state so we can apply move to it
		jobject stateCopy = jenv->NewGlobalRef(jenv->NewObject(clsLudiiStateWrapper, midLudiiStateWrapperCopyCtor, current->wrappedState));
		jenv->CallVoidMethod(stateCopy, midApplyMove, unexpandedMove);

		// No longer need this ref
		jenv->DeleteGlobalRef(unexpandedMove);

		// Create expanded node and return it
		current->childNodes.emplace_back(jenv, current, stateCopy, current->scoreSums.size(), rng);
		return &(current->childNodes.back());
	}

	// Use UCB1 equation to select from all children, with random tie-breaking
	MCTSNode* bestChild = nullptr;
	double bestValue = std::numeric_limits<double>::lowest();
	const double twoTimesParentLog = 2.0 * std::log(std::max(1U, current->visitCount));
	int32_t numBestFound = 0;

	size_t numChildren = current->childNodes.size();
	int32_t mover = (int32_t) jenv->CallIntMethod(current->wrappedState, midCurrentPlayer);

	for (size_t i = 0; i < numChildren; ++i) {
		MCTSNode* child = &(current->childNodes[i]);
		const double exploit = child->scoreSums[mover] / child->visitCount;
		const double explore = std::sqrt(twoTimesParentLog / child->visitCount);

		const double ucb1Value = exploit + explore;

		if (ucb1Value > bestValue) {
			bestValue = ucb1Value;
			bestChild = child;
			++numBestFound;
		}
		else if (ucb1Value == bestValue) {
			std::uniform_int_distribution<std::mt19937::result_type> dist(0, numBestFound + 1);

			if (dist(rng) == 0) {
				bestChild = child;
			}

			++numBestFound;
		}
	}

	return bestChild;
}

JNIEXPORT jobject JNICALL Java_ludii_1cpp_1ai_LudiiCppAI_nativeSelectAction
(JNIEnv* jenv, jobject jobjAI, jobject game, jobject context, jdouble maxSeconds,
		jint maxIterations, jint maxDepth)
{
	std::random_device dev;
	std::mt19937 rng(dev());

	jobject wrappedGame = jenv->NewObject(clsLudiiGameWrapper, midLudiiGameWrapperCtor, game);
	jobject wrappedRootContext = jenv->NewObject(clsLudiiStateWrapper, midLudiiStateWrapperCtor, wrappedGame, context);

	const int32_t numPlayers = (int32_t)jenv->CallIntMethod(wrappedGame, midNumPlayers);
	auto root = std::make_unique<MCTSNode>(jenv, nullptr, wrappedRootContext, numPlayers, rng);

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

			current = Select(jenv, current, rng);
		}
	}

	// Clean up refs to Java objects we created
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

