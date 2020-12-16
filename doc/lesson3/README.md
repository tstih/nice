# Let's Paint!

Hello again. In previous two lessons we learned how to create
platform independent application class and window class. In this
chapter we are going to process some event's and paint some
lines to its surface.

Our starting point will be, as usual, the code that we wrote 
in previous lesson. 

## Signals and slots

You may have heard about signals and slots. It's a cool new
mechanism to delegate events in C++. 

It was W. H. Davenport Adams who first said: "Good artists copy; 
great artists steal!" Not Steve Jobs, and not Pablo Picasso. 
We are going to be good artists, and reuse an existing [signals
and slots implementation](https://schneegans.github.io/tutorials/2015/09/20/signal-slot)  
by Simon Schneegans. His code follows the nice philosophy: it is short, 
and utilizes modern C++11 (variadic templates).

So without any further talk here is the stripped version of his code.

~~~cpp
~~~