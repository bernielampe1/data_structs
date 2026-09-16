This is a pedagogical set of data structs, and some algorithms in C++. We concentrated on the rule-of-five implementations. The rule-of-zero is a better way to do this so you don't have to re-accomplish work and inevitably introduce bugs. These are used for learning and teaching.

Rule-of-five-default:
    If you define, =default or =delete any of dtor, copy ctor, assignment op, move ctor, move assignment op then you must define, =default or =delete them all.

Rule-of-zero:
    Use value semantics of stl and smart pointers to ensure you don't have to define any of the copy ctor, move ctor, assignment op, move assignment op, or dtor.

Rule-of-five:
    If you define any of the copy ctor, move ctor, assignment op, move assignment op, or dtor, you must to all five.

Rule-of-three:
    If you define any of the copy ctor, assignment op, or dtor, you must to all three.
