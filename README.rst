plug
====

**This is just a proof-of-concept—it is not intended for use in-production.**
**Use at your own risk.**

It is intended to be inspirational and instructive; it is not intended to be production-quality code.
The above in-mind, some steps have been taken to be reasonably portable and stable.

:code:`plug` is intended to serve as a framework for a configurable status bar, for the moment.
It will eventually allow for callbacks so that you can have it do whatever you want with the plugin output; but, for the moment, it will support writing plugin output to the X root window's name (for WMs like dwm), or stdout.

tl;dr
-----

The :code:`dlopen()` family of functions is the POSIX-compliant method for loading a shared library at runtime.
Therefore, if you are looking for a simple, portable system for modules, this is the way.

features
--------

* Discovers and loads an arbitrary number of plugins
* Automatically allocates a buffer according to each module's requested size
* Outputs the results of each module

api
---

Modules may expose whatever symbols they would like; however, there are only a few symbols which are resolved by the loader:

* :code:`size` (the size of the buffer that should be allocated for the plugin)
* :code:`interval` (the interval (in seconds) for when :code:`play` should be called; :code:`0` disables the module)
* :code:`setup` (an initialization function run when loading each plugin, *optional*)
* :code:`play` (the main function of the plugin, expected to finish either by early-exit in the case of failure or by writing the plugin's formatted state to the buffer argument)
* :code:`teardown` (a cleanup function run when unloading each plugin, *optional*)

