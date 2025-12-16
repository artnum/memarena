# memarena : arena allocator

A simple arena allocator for general purpose with some tweaks for dynamic array
and string maniuplation.

## About realloc and free

As it's a bump allocator, you can just malloc and then destroy the arena. But,
in some case, you can use realloc and free without high penalities. As an
example, if you do a dynamic array, you might be tempted to use it as it could
be faster than classic realloc (see test/memarena.c:295).

Also it is faster on malloc/free (see test/memarena.c:251).
