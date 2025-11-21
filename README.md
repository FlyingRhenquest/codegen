# Codegen

This is a little demo project that parses enum data from a C++ file and
uses the information to generate to_string functions and ostream operators
from the enum information. A *very* basic example demonstrates this in 
action.

This is not indended to be a production-ready project. Rather, it
demonstrates how to use boost::spirit::x3 for a somewhat complex
parser. The parser itself is fairly fragile at the moment. It does not
claim to conform 100% to the C++ standard for even the limited amount
of stuff that it does. It is a good jumping off point to push further
into C++ parsing and I do intend to add some more features to it.

If you actually want to use this code to generate operators for
your enums, it would be best to put your enums in a separate header
that you can run the GenerateEnumFunctions against. That will give the
program the best chance to succeed. If you're doing anything with
your enums that is weird enough to make the parser fail, please file an
issue in the github project and I'll try to adjust the code to handle it.

This project is licensed under Apache 2.0. If you do something interesting
with it, I'd love to hear about it!

# Limitations

This code won't generate code for anonymous enums, enums embedded in
enums or enums embedded in classes. It hasn't been tested with anything
more complex than the enums.h file in the examples/enum_examples directory.

# Building

This program requires boost::spirit::x3, boost::program_options and
boost::signals2. If you have a fairly recent boost installed somehwere
that CMake can find it, you should be able to build it with:

```
mkdir /tmp/build
cd /tmp/build
cmake ~/sandbox/codegen
sudo make install
```
 
The example should be built separately once the GenerateEnumFunctions
program has been installed in your path:

```
 mkdir /tmp/build\_examples
 cd /tmp/build\_examples
 cmake ~/sandbox/codegen/examples/enum\_example
 make
 ./enum\_example
```

If you modify the parser and rebuild GenerateEnumFunctions, you'll need
to rerun cmake for the examples. I plan to add some CMake integration
for the code generation but have not done so yet.

If you want to build and run the tests, specify -DBUILD_TESTS=ON
in the cmake command line.

If you set CMAKE\_BUILD\_TYPE=DEBUG, this will enable the boost
spirit X3 XML debug output, which can be really handy for seeing
what your parser's doing.

# My Conclusions from this project

boost::spirit::x3 is actually pretty easy to use once you get over the
initial learning curve. Don't let the library intimidate you. It only
took me a couple of days to put this together and I had almost no 
experience with boost x3 prior to this.

It is best to develop your parser one X3 rule at a time and then write
a somewhat comprehensive test to validate that the rule is being
matched and processed the way you want it to be. Building parsers can
still be fairly tricky. If you have a language specification you're
working from, that can help immensely with laying out your token
definitions. I didn't bother to go check the C++ standard and instead
went from my own knowledge of the language, so this probably won't
conform 100% to the C++ standard. That wasn't really the point of the
exercise.

Documentation and forum support for this particuar library can be
somewhat iffy since you'll match like 3 other parser libraries that
boost contains when you're searching. One particular issue I ran
across was that tokens were being concatinated when I hit them
multiple times in the parser. So my first namespace test initially
returned the tokens "foo", "foobar" and "foobarbaz." Stack overflow
and Google's gemini AI suggested using x3::at to solve this problem,
but x3::at does not and apparently has never existed. The correct
solution was to reset the x3 context after storing the informationm I
need, like so:

 x3::_attr(ctx) = "";
 
This information was not documented anywhere, it just occurred to me to try
it.

If you just want to grab some tokens out of a text stream, check out
the signals exported in the ParserDriver object defined in parser.h.
Any number of things can subscribe to those signals to be notified of
specific events. This can make processing somewhat tricky, but it
can also let you isolate processing for specific tasks for much
neater encapsulation that would have been previously possible. The
namespace object and the enum data object in data.h are both interested
in namespaces, for example, but do their own processing based on the
information they receive. So unless I start breaking encapsulation
by copying redundant information across objects, I can be pretty
sure the namespace processing will always be correct. I can query that
object or a container of those objects when I need anything relating to
namespaces.

One quick note on boost signals2 signals; the signal handler runs in
the caller's thread. So your signal handler will block the parser
until the handler returns. If you need to do a significant amount of
processing and need real-time responses from other objects monitoring
the parser, you'll want to copy the data you receive and handle it in
a different thread. It's up to you to make a call on when the processing
in your signal handler crosses the "Too much to block the parser"
threshold, but it's always a good thing to be aware of when working
with boost signals2 signals. My media and media2 projects do a lot
of that sort of processing if you're curious what the code looks
like for handling that.

# Future Plans

I want to provide CMake integration so you can generate ostream operators
and to_string functions and have the generated code automatically
added to your target. This is not difficult, just mildly time consuming
if you want it to be reasonably reliable.

I want to clean up the data.h code. This would mainly involve implementing
constructors and destructors and moving some data to private. Having
a queryable API would be a lot nicer for future expansions of this project.

I want to add a python API to the data objects and ParserDriver. Maybe even
include a python API call to the code generator.

I'd like to extend this to read simple C++ classes and structs. Nothing
templated, lol. At the very least, adding a cereal serialize function generator
for plain old data structs would be almost trivial at this point.
It would be only slighty more difficult to add a serialize function
generator for more complex classes right up until templates become involved.
A lot of template cases don't make sense to serialize anyway, so this
should still be fairly useful.
