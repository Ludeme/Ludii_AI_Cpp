package ludii_cpp_ai;

import app.StartDesktopApp;
import manager.ai.AIRegistry;

/**
 * Class with main method to launch Ludii's GUI with our C++-based AI registered.
 * 
 * @author Dennis Soemers
 */
public class Main 
{
	
	/**
	 * Our main method.
	 * 
	 * @param args
	 */
	public static void main(final String[] args)
	{
		AIRegistry.registerAI("C++ AI", () -> {return new LudiiCppAI();}, (game) -> {return new LudiiCppAI().supportsGame(game);});
		
		StartDesktopApp.main(new String[0]);
	}

}
