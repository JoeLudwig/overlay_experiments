What
----

These experiments use Awesomium to show HTML pages as OpenVR dashboard overlays. Nothing here is a finished product, but hopefully they'll inspire someone to build something that is.

All the original code here is licensed under the BSD 3-clause license. I encourage you to clone this repo and use for whatever you like (within the fairly permissive confines of that license.)  This repo depends on SDL, Awesomium, OpenVR, and GLEW, so obviously you'll have to think about those licenses too. Awesomium you'll need to install from [their website](http://awesomium.com). Everything else is in the repo.

Why
---
I think that overlays in OpenVR have the potential to be very useful for a bunch of things in VR, but they haven't really taken off. There are a few out there (OVRDrop, OpenVR Advanced Settings, Viveport, etc.) but there should be many more.

I'm making this available for two reasons:
* To make it easier to get a custom overlay up and running if somebody wants to try something out.
* To try out some ideas myself that might inspire somebody to build an actual product.

If I come up with something particularly useful I may make binaries available. In the meantime, feel free to build this and host binaries somewhere. Or take this code and make a product of your own. If you do that and want to offer it on Steam, let me know.

A note about web security
-------------------------

The web is full of bad actors and malware. Chrome (and other browsers) fight against that with frequent updates that patch every hole they find. Although Awesomium uses the same rendering engine as Chrome it does not get updated at the same rate. And even if it did, you would need to then get (or build) an update for any binary you have based on it. 

Use caution when using anything based on this stuff on random web pages out in the wild. Local html files are safe enough, but you don't need to get very far off the beaten path to find a site that has had malware in one of its ads at some point. Using anything in this repo as the basis for a general-purpose web browser is probably a bad idea. 

TODO
----

I don't know if I'll actually do any of these, but here's stuff that isn't done yet:
* Multi-platform. I've only built on Windows. OpenVR also supports Linux and OSX.
* Thinking about cookies/local config. I just stick a file in your My Documents directory, which isn't even the right place on Windows these days.
* Javascript hooks for OpenVR APIs like IVRSettings and IVRApplications.
* Awesomium doesn't seem to have a way to provide a texture that's already on the GPU (preferably having used the GPU for web rendering.) That extra CPU->GPU copy on every frame is adding a bunch of overhead that will probably make WebGL perform poorly, and isn't great for things like video.

