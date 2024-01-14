# GOAL: Demonstrate how to use a third-party python module

# Load bind from eggdrop, not eggdrop.tcl. Loading it from eggdrop.tcl would cause
# the bind to call a Tcl proc, not the python function.
from eggdrop import bind

# Load any Tcl commands you want to use.
from eggdrop.tcl import putmsg, putlog

# Load a python module as usual.
from imdb import Cinemagoer

# Just a totally normal Python function, nothing special here! Note the Tcl command 
# 'putlog' is called
def pubGetMovie(nick, user, hand, chan, text, **kwargs):
    imdb = Cinemagoer()
    putlog("IMDB: Searching for "+text)
    results = imdb.search_movie(text)
    movie = imdb.get_movie(results[0].movieID)
    putmsg(chan, f"Movie: {movie['title']} ({movie['year']}): {movie['plot outline']}")

# Call binds at the end of the script, not the top- the functions must be defined!
bind("pub", "*", "!movie", pubGetMovie)

# Should we talk about print vs putlog?
print('Loaded imdb.py')
