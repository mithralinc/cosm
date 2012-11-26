----------------------------------------------
CS-SDK, Client-Server Software Development Kit
----------------------------------------------

This client-server SDK and example code will allow you to develop stand-alone
software for use in client-server applications.

Putting this out was prompted by the many people that have contacted me
looking for a fast way to develop client-server apps for their research
projects. Most of them need only the simple services a client-server
architecture provides, but still need the cross-platform and fast development
that the Cosm libraries provide. This kit will allow the easy development of
projects similar to what d.net or SETI run.

Developing a client-server application using the CS-SDK
-------------------------------------------------------

The long version
----------------

Read cs-sdk.html

The short version
-----------------

1. Learn about client-server architectures and design your application.
   If you skip this step, you wont get any further.
2. This package requires the Cosm libraries, API, and coding style to be used.
   Build the libraries and read the Cosm API documentation and
   Cosm Programmers Guide available on http://www.mithral.com/developers/
3. Read through all of the .c and .h files.
4. Decide if you want to use normal TCP or HTTP tunneling, currently the
   cs-sdk is coded to support _both_. Obviously on a LAN you can remove the
   HTTP code, but on the net, you may only want HTTP.
5. Replace the example code with yours and add your code.
6. Edit the Makefile, adding any additional files you need.
7. Type `make`. If the Cosm libraries compiled correctly, this will work.

