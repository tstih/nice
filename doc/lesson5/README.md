# It's nice to have children

They say children are our greatest treasure. This lesson 
is dedicated to managing children and keeping them in order.
In it we will create a layout manager that will handle two 
child controls (a button and a label).

# Extend the window class

So far our event handling was more of a proof of concept
then a serious undertaking. Before embarking on this quest, 
let's enrich our core window by adding:
 * ability to read and write window size,
 * ability to read and write window position,
 * resize event handler,  
 * ability to trigger naive window repaint, and
 * event handlers for keyboard and mouse,
 * naive text output.

But only after we get familiar with a few patterns.

# The property pattern

To add some of the required features to the window class,
let's add *proper properties*. They will beave just like
variables; but when read from or write to - they'll call 
functions to accomplish the task. 

We will be able to write

~~~cpp
w.title="Hello!";
w.size={640,400};
~~~

instead of

~~~cpp
w.set_title("Hello!");
w.set_size({640,400});
~~~

Here's the class that enables us to define a property.

~~~cpp
template<typename T>
class property {
public:
    property(
        std::function<void(T)> setter,
        std::function<T()> getter) :
        setter_(setter),getter_(getter) { }
    operator T() const { return getter_(); }
    property<T>& operator= (const T &value) { setter_(value); return *this; }
private:
    std::function<void(T)> setter_;
    std::function<T()> getter_;
};
~~~

If you'd like to enable overriding a property, it should 
call a protected virtual setter and getter. Let's respect 
the following convention for these functions: a protected 
setter and getter function shall be called the same as
the property variable, but prefixed with a set_ or a get_. 
For example `set_size` and `get_size` for property `size`. 

If this theory sounds a bit complex, here's how to do it in
practice.

~~~cpp
class wnd : public resource<wnd_instance> {
    // ... code omitted ...
public:
    property<std::string> title {
        [this] (std::string s) { this->set_title(s); },
        [this] () -> std::string {  return this->get_title(); }
    };
};
protected:
    virtual std::string get_title() {} // Implement it!
    virtual void set_title(std::string s) {} // Implement it!
    // ... code omitted ...
}
~~~

So for each property - we write plain getter and setter, and then 
simply add the variable with lambdas that call these functions.
If we want to override it, we override the protected getter and/or setter.

 > In future, we may refactor the `property` class to derivate 
 > from a `read_property` and `write_property` classes, each implementing
 > half of functionality, read only and write only properties.

# The unit pattern

Our layout manager, and text output will both require 
handling different units, such as percents, pixels, ems, etc. 
A nice solution to this is using [user-defined literals](https://www.geeksforgeeks.org/user-defined-literals-cpp/), which have been around since C++11. 
User defined literals will allow us to write quantities, such as `50_pc`, `200_px`,
or `1.5_em`.

 > Why the underscore? Wouldn't it look better if we define 
 > without it ie.. `50pc`,`200px`, and `1.5em`? It most
 > definitely would. But to assure that our software works
 > with any future standard library, literals without underscores
 > should only be defined by the standard library.

# Handling window size 

Time to implement the size property, and the resized event.

We begin by adding a new signal, a getter function, a setter function,
and a property to our wnd class.

~~~cpp
class wnd : public resource<wnd_instance> {
    // ... code omitted ...
public:
    signal<> resized;
    property<size> wsize {
        [this] (size sz) { this->set_wsize(sz); },
        [this] () -> size {  return this->get_wsize(); }
    };
};
protected:
    virtual std::string get_wsize(); 
    virtual void set_wsize(std::string s); 
    // ... code omitted ...
}
~~~

 > In this lesson we will **not** be showing how to implement  all new 
 > properties (`wsize`, `title`, and `location`) and their events.  
 > One example is enough. If you want to understand others - read the code. 