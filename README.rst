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
* Automatically allocates a buffer according to each module's requested size
* Outputs the results of each module

api
---

Modules may expose whatever symbols they would like; however, there are only a few symbols which are resolved by the loader:

* :code:`size` (the size of the buffer that should be allocated for the plugin)
* :code:`priority` (the position the plugin should reside in the list (1 is left-most; the order plugins appear in the list when they have the same priority is not guaranteed)
* :code:`setup` (an initialization function run when loading each plugin, *optional*)
* :code:`play` (the main function of the plugin, expected to finish either by early-exit in the case of failure or by writing the plugin's formatted state to the buffer argument)
* :code:`teardown` (a cleanup function run when unloading each plugin, *optional*)

