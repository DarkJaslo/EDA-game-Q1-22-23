# EDA-game-Q1-22/23

<h1>
  Tropotron ( The Walking Dead - FIB - Q1 2022-2023 )
</h1>


Welcome! This was my submit for UPC's Computer Engineering (FIB)'s subject EDA's game, carried out in November-December 2022. I'm glad it made it to the final.

Contains my bot (Tropotron) and all required files to run the game in /game. Please note that I just made <tt>AITropotron.cc</tt> and the game tester <tt>tester.cc</tt>. Everything else is credited in <tt>README.txt</tt> inside /game.

/game also contains all information regarding the rules and running matches, as well as the game's API, in a couple .pdf files.


<h3> Important notes </h3>

- /game/aux contains two .o files that get deleted when using <tt>make clean</tt>, so you can easily copy them back if the game does not compile.
- You can use my tester (<tt>tester.cc</tt> located in /game) to run matches and compare bots. Compile <tt>tester.cc</tt> and run it with this format: <tt>./tester <number_of_games> <name_1> <name_2> <name_3> <name_4> <number_of_cores></tt>, where <tt>name_x</tt> is just the name of the bot. For instance, if your bot is called AITropotron.cc, its name would be Tropotron. Regarding <tt>number_of_cores</tt>, it should be the number of threads your CPU supports, as this program uses very simple parallel code and it needs this information to work properly.
- The games run slowly! Don't expect the <tt>tester</tt> to be able to run many tests in a couple of seconds. I used a fairly powerful PC to run my own tests, and 10000 tests would take about half an hour using just my bot (which happens to be quite optimized). I recommend running 100 to have an approximate idea and if results still aren't clear, use 500 or 1000. With 1000 or more, the statistical noise gets low enough.
- You can match bots against Dummy, which was considered by our professors as the bare minimum we had to reach to pass. If you really want to waste your time (and electricity), try running games with Tropotron and three Dummies until a Dummy wins.
- You may be able to find more information (which might not be useful to you) inside <tt>Tropotron.cc</tt> or <tt>tester.cc</tt>.
