# DagBox Coding Style

[Qt_style]: <https://wiki.qt.io/Qt_Coding_Style>

This coding style has been inspired by [Qt's Coding Style][Qt_style].


## Indentation

The code should be indented with 4 spaces.


## Declarations

When naming, use `snake_case`. Generally, descriptive names should be
preferred, but short names are acceptable for short-lived variables.

When declaring variables, each should be declared on a separate
line. The declaration should be done as close as possible to the point
where the variable is used.

```
// Wrong
int client_count, worker_count;

// Correct
int client_count;
int worker_count;
```

Functions should be declared using trailing return types.

```
// Wrong
double calculate_foo(int bar);

// Correct
auto calculate_foo(int bar) -> double;
```


## Whitespace

Always insert a single space after a keyword and before a curly brace.

```
// Wrong
if(test){
}

//Correct
if (test) {
}
```

For pointers and references, always use spaces around `*` and `&`.

```
// Wrong
char *address;

// Correct
char * address;
```

Place `const` keyword on the right of whatever it applies to.

```
int const * x; // Mutable pointer to constant integer
int * const x; // Constant pointer to mutable integer
```

Binary operators should be surrounded with spaces.

C-style casts should be avoided as much as possible.

```
// Wrong
char * buffer = (char *)malloc(256);

// Correct
char * buffer = reinterpret_cast<char *>(malloc(256);
```

Never put multiple statements in a single line.


## Braces

Use attached braces, except for function and class declarations.

```
if (test) {
} else {
}
```

Curly braces should always be used with conditional statements, even
when they contain only one line.

```
// Wrong
for (auto x : xs)
    cout << x;
    
// Correct
for (auto x : xs) {
    cout << x;
}
```


## Parentheses

Use parentheses to group expressions whenever the expressions are
ambiguous without operator precedence, except for basic mathematical
operators.

```
// Wrong
a + b & c

// Correct
(a + b) & c
x + y * z
```


## Switch Statements

Case labels should have the same indentation as the switch
statement. Every case should end with `break`, `return` or a comment
noting that the fall through is intentional, unless another case
follows immediately.

```
switch (socket_type) {
case ZMQ_PUB:
    do_work();
    break;
case ZMQ_SUB:
case ZMQ_REQ:
    other_work();
    return 10;
case ZMQ_REP:
    something_else();
    // fall through
default:
    default_work();
    break;
}
```


## Line Breaks

Lines should be shorter than 80 characters. Wrap if necessary. If
there are too many nested blocks, consider breaking it up into
separate functions. On wrapped lines, commas go at the end and
operators at the start.

```
function_with_a_long_name(some_variable,
                          long_expression
                          + with_operators);
```


## Comments

[doxygen]: <http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html>
[doxygen_markdown]: <http://www.stack.nl/~dimitri/doxygen/manual/markdown.html>

Everything that is top level should have a comment attached. These
comments should use Qt style [Doxygen comment blocks][doxygen],
using [markdown][doxygen_markdown]. If the comment is attached to
something that is publicly accessible (part of the public API), it
should include code samples or links to other parts of the
documentation that have code samples.

```
/*! \brief Brief description.
 * 
 * Detailed description.
 */
```


## Classes

[rule_of_5]: <https://en.wikipedia.org/wiki/Rule_of_three_(C++_programming)#Rule_of_Five>

Classes should follow the [rule of 5][rule_of_5]. If a class defines a
custom destructor, copy costructor, move constructor, copy assignment
operator or move assignment operator; then it should either define or
explicitly delete others.

If it is unsafe or inefficient to copy the class, the copy constructor
and the copy assignment operator of the class should be deleted to
prevent it from being accidentally copied.
