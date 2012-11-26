
Building libCosm
----------------

Type `build help` and follow the directions. Then run src/textlib.exe.
Platform specific notes are below.


Porting Cosm
------------
Cosm is designed to port to any machine with a 32bit CPU or better.

1. Copy the TEMPLATE makefile in make/ to the correctly named os-cpu
   file. Edit the makefile for your platform.

2. Type `build os-cpu` in the main dir.

3. Read through all CPU/OS layer files, port any code that doesn't
   compile, taking care not to break any other platforms.

4. Run src/testlib.exe and fix any detected faults.

5. Add any building notes to this file.


CPU/OS Layer
------------
src/
cputypes.h      os_io.c         os_math.h       os_net.c        os_task.h
os_file.c       os_io.h         os_mem.c        os_net.h
os_file.h       os_math.c       os_mem.h        os_task.c

These are the only files in Cosm that need to be ported to new platforms.
Once they are ported and the proper makefile is made, all other functions
should compile cleanly. Outside of these files NO platform specific
functions should be used, only the ones in the Cosm interface. In this
way all future code will require no porting.


Utility Layer
-------------

Everything else in /src (not in subdirectories) is the Utility later.
Code that is built on top of the CPU/OS layer, and part of the core
Cosm library.


2nd Party, 3rd Party
--------------------
"2nd party" libraries are other software packages included in libCosm and
whose headers get installed as part of the default install. They are
subdirectories in the /src directory. 3rd party libraries are added after
Cosm is installed, but are configured to be API and directory layout
compatible with Cosm when installed.


Platform Specific Notes
-----------------------

Linux:
  Most glibc based systems will have support for threads (libpthread)
  however, most older libc5 and, older still, libc4 systems will most
  likely _not_ have thread support. You will need to install the
  linuxthreads package.

Win32:
  The win32-x86-msvc makefile now works, but you will need a couple helper
  applications beyond just nmake. rm.exe, and strip.exe are needed from the
  cygwin tools. You also have to manually do what build does for now,
  `copy make\win32-x86-msvc Makefile.cosm` and the install to \usr\local\*.
  The sub-makefiles also need to have the "-L" changed to "/LIBPATH:" and
  the "-lCosmUtil" to "libCosmUtil.a". We'll have a perl script for that
  change soon.
