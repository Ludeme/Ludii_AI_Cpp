package ludii_cpp_ai;

import game.Game;
import other.AI;
import other.context.Context;
import other.move.Move;


/**
 * Example Java wrapper for a Ludii AI implemented in C++
 *
 * @author Dennis Soemers
 */
public class LudiiCppAI extends AI
{
	
	//-------------------------------------------------------------------------
	
	/** Our player index */
	protected int player = -1;
	
	//-------------------------------------------------------------------------
	
	/**
	 * Constructor
	 */
	public LudiiCppAI()
	{
		this.friendlyName = "Example Ludii C++ AI";
	}
	
	//-------------------------------------------------------------------------

	@Override
	public Move selectAction
	(
		final Game game, 
		final Context context, 
		final double maxSeconds,
		final int maxIterations,
		final int maxDepth
	)
	{
		// TODO call C++ AI
		return null;
	}
	
	@Override
	public void initAI(final Game game, final int playerID)
	{
		this.player = playerID;
	}
	
	//-------------------------------------------------------------------------

}
