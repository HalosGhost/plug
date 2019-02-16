dyload
======

**This is a proof-of-concept.**

It is intended to be inspirational and instructive; it is not intended to be production-quality code.
The above in-mind, some steps have been taken to be reasonably portable and stable.

tl;dr
-----

The :code:`dlopen()` family of functions is the POSIX-compliant method for loading a shared library at runtime.
Therefore, if you are looking for a simple, portable system for modules, this is the way.

features
--------

* Can load up to 100 modules (could easily be increased with a discovery step)
* Modules can expose whatever symbols they would like (but must expose :code:`modsize` and :code:`modstep`)
* Automatically allocates a buffer according to each module's requested size
* Outputs the results of each module

