# Pepo v2.0.0 Changes

Added spell queuing with lots of customization.  See nampower.cfg for configuration details.

### How does queuing work?
Trying to cast a spell within the appropriate window before your current spell finishes will queue your new spell.  
The spell will be cast as soon as possible after the current spell finishes.  


### Why do I need a buffer?
From my own testing it seems that a buffer is required on spells to avoid "This ability isn't ready yet" errors.  
By that I mean that if you cast a gcd spell every 1.5 seconds without your ping changing you will occasionally get 
errors from the server and your cast will get rejected.  If you have 150ms+ ping this can be very punishing.  

I believe this is related to the time it takes the server to process incoming spells.  There is logic to 
subtract the server tick time from your gcd in vmangos but turtle does not appear to be doing this.

# Pepo v1.0.0 Changes

Now looks for a nampower.cfg file in the same directory with two lines:

1.  The first line should contain the "buffer" time between each cast.  This is the amount of time to delay casts to ensure you don't try to cast again too early due to server/packet lag and get rejected by the server with a "This ability isn't ready yet" error.  For 150ms I found 30ms to be a reasonable buffer.
2.  The second line is the window in ms before each cast during which nampower will delay cast attempts to send them to the server at the perfect time.  So 300 would mean if you cast anytime in the 300ms window before your next optimal cast your cast will be sent at the idea time.  This means you don't have to spam cast as aggressively.  This feature will cause a small stutter because it is pausing your UI (I couldn't find a better way to do this but I'm sure one exists now with superwow) so if you don't like that set this to 0.

**Please consider donating if you use this tool. - Namreeb**

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=QFWZUEMC5N3SW)
  
# nampower

An auto stop-cast tool for World of Warcraft 1.12.1.5875 (for Windows)

There is a design flaw in this version of the client.  A player is not allowed to cast a
second spell until after the client receives word of the completion of the previous spell.
This means that in addition to the cast time, you have to wait for the time it takes a
message to arrive from the server.  For many U.S. based players connected to E.U. based
realms, this can result in approximately a 20% drop in effective DPS.

Consider the following timeline, assuming a latency of 200ms.

* t = 0, the player begins casting fireball (assume a cast time of one second or 1000ms)
       and spell cast message is sent to the server.  at this time, the client places
       a lock on itself, preventing the player from requesting another spell cast.
* t = 200, the spell cast message arrives at the server, and the spell cast begins
* t = 1200, the spell cast finishes and a finish message is sent to the client
* t = 1400, the client receives the finish message and removes the lock it had placed
          1400ms ago.
		  
In this scenario, a 1000ms spell takes 1400ms to cast.  This tool will work around that
design flaw by altering the client behavior to not wait for the server to acknowledge
anything.

## New In Version 2.0 ##

Previous versions of this tool required custom macros.  This version should "just work".

## Using ##

If you use my launcher, known as [wowreeb](https://github.com/namreeb/wowreeb), you can add
the following line to a `<Realm>` block to tell the launcher to include this tool:

```xml
    <DLL Path="c:\path\to\nampower.dll" Method="Load" />
```

To launch with the built-in launcher, run loader.exe -p c:\path\to\wow.exe (or just loader.exe
with it inside the main wow folder)

### Account Security ###

While this makes no malicious changes to the WoW client, it could easily be mistaken
as malicious by the primitive anticheats in use on some vanilla private servers.
This program contains absolutely no protection against anticheat software.

Having written the anticheat for Elysium and Light's Hope, I can say that they do not
currently detect this, and are unlikely ever to do so.

Kronos / Twinstar has said that while they do not support client modification, they
will not specifically target this mod.  Refer to this thread:
http://forum.twinstar.cz/showthread.php/97154-Planning-to-release-a-wow-mod-Will-it-get-people-banned

USE AT YOUR OWN RISK
