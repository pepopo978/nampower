# Pepo v2.0.0 Changes

Added spell queuing, automatic retry on error, and quickcasting with lots of customization.  

### Configuration
The following CVars control the behavior of the spell queuing system:

You can access CVars in game with `/run DEFAULT_CHAT_FRAME:AddMessage(GetCVar("CVarName"))`<br>
and set them with `/run SetCVar("CVarName", "Value")`

You can also just place them in your Config.wtf file in your WTF folder.  If they are the default value they will not be written to the file.
Example:
```
SET EnableMusic "0"
SET MasterSoundEffects "0"

SET NP_QuickcastTargetingSpells "1"
SET NP_SpellQueueWindowMs "1000"
SET NP_TargetingQueueWindowMs "1000"
```

- `NP_QueueCastTimeSpells` - Whether to enable spell queuing for spells with a cast time.  0 to disable, 1 to enable. Default is 1.
- `NP_QueueInstantSpells` - Whether to enable spell queuing for instant cast spells tied to gcd.  0 to disable, 1 to enable.  Default is 1.
- `NP_QueueOnSwingSpells` - Whether to enable on swing spell queuing.  0 to disable, 1 to enable. Default is 1.
- `NP_QueueChannelingSpells` - Whether to enable channeling spell queuing.  0 to disable, 1 to enable. Default is 1.
- `NP_QueueTargetingSpells` - Whether to enable terrain targeting spell queuing.  0 to disable, 1 to enable. Default is 1.


- `NP_SpellQueueWindowMs` - The window in ms before a cast finishes where the next will get queued. Default is 500.
- `NP_OnSwingBufferCooldownMs` - The cooldown time in ms after an on swing spell before you can queue on swing spells. Default is 500.
- `NP_ChannelQueueWindowMs` - The window in ms before a channel finishes where the next will get queued. Default is 1500.
- `NP_TargetingQueueWindowMs` - The window in ms before a terrain targeting spell finishes where the next will get queued. Default is 500.


- `NP_MinBufferTimeMs` - The minimum buffer delay in ms added to each cast (covered more below).  The dynamic buffer adjustments will not go below this value. Default is 55.
- `NP_NonGcdBufferTimeMs` - The buffer delay in ms added AFTER each cast that is not tied to the gcd. Default is 100.
- `NP_MaxBufferIncreaseMs` - The maximum amount of time in ms to increase the buffer by when the server rejects a cast. This prevents getting too long of a buffer if you happen to get a ton of rejections in a row. Default is 30.


- `NP_RetryServerRejectedSpells` - Whether to retry spells that are rejected by the server for these reasons: SPELL_FAILED_ITEM_NOT_READY, SPELL_FAILED_NOT_READY, SPELL_FAILED_SPELL_IN_PROGRESS.  0 to disable, 1 to enable. Default is 1.
- `NP_QuickcastTargetingSpells` - Whether to enable quick casting for ALL spells with terrain targeting.  This will cause the spell to instantly cast on your cursor without waiting for you to confirm the targeting circle.  Queuing targeting spells will use quickcasting regardless of this value (couldn't get it to work without doing this).  0 to disable, 1 to enable. Default is 0.
- `NP_ReplaceMatchingNonGcdCategory` - Whether to replace any queued non gcd spell when a new non gcd spell with the same StartRecoveryCategory is cast (more explanation below).  0 to disable, 1 to enable. Default is 0.

### Bug Reporting
If you encounter any bugs please report them in the issues tab.  Please include the nampower_debug.txt file in the same directory as your WoW.exe to help me diagnose the issue.  If you are able to reproduce the bug please include the steps to reproduce it.  In a future version once bugs are ironed out I'll make logging optional.

### How does queuing work?
Trying to cast a spell within the appropriate window before your current spell finishes will queue your new spell.  
The spell will be cast as soon as possible after the current spell finishes.  

There are separate configurable queue windows for:
- Normal spells
- On swing spells (the window functions as a cooldown instead where you cannot immediately double queue on swing spells so that I don't have to track swing timers)
- Channeling spells
- Spells with terrain targeting

Only one gcd spell can be queued at a time.  Pressing a new gcd spell will replace any existing queued gcd spell.

Non gcd spells have special handling.  You can queue up to 5 non gcd spells, 
and they will execute in the order queued with `NP_NonGcdBufferTimeMs` delay after each of them to help avoid server rejection.  
The non gcd queue always has priority over queued normal spells.

`NP_ReplaceMatchingNonGcdCategory` will cause non gcd spells with the same StartRecoveryCategory to replace each other in the queue.  
The vast majority of spells not on the gcd have category '0' so enabling this setting will cause them to all replace each other.  
One notable exception is shaman totems on TWoW that were changed to have separate categories according to their elements.

This can be useful if you want to change your mind about the non gcd spell you have queued.  For example, if you queue a mana potion and decide you want to use LIP instead last minute.
However, this will also prevent using certain cooldowns together with trinkets, such as Combustion followed by Mind Quickening Gem.  Decide what works best for you.

### Why do I need a buffer?
From my own testing it seems that a buffer is required on spells to avoid "This ability isn't ready yet"/"Another action in progress" errors.  
By that I mean that if you cast a gcd spell every 1.5 seconds without your ping changing you will occasionally get 
errors from the server and your cast will get rejected.  If you have 150ms+ ping this can be very punishing.  

I believe this is related to the server tick during which incoming spells are processed.  There is logic to 
subtract the server processing time from your gcd in vmangos but turtle does not appear to be doing this.  

To compensate for what seems to be a 50ms server tick the default buffer in nampower.cfg is 55ms.  If you are close to the server
you can experiment with lowering this value.  You will occasionally get errors but if they are infrequent enough for you
the time saved will be worth it.  

Non gcd spells also seem to be affected by this.  I suspect that only one spell can be processed per server tick.  
This means that if you try to cast 2 non gcd spells in the same server tick only one will be processed.  
To avoid this happening there is `NP_NonGcdBufferTimeMs` which is added after each non gcd spell.  There might be more to
it than this as using the normal buffer of 55ms was still resulting in skipped casts for me.  I found 100ms to be a safe value.

# Pepo v1.0.0 Changes

Now looks for a nampower.cfg file in the same directory with two lines:

1.  The first line should contain the "buffer" time between each cast.  This is the amount of time to delay casts to ensure you don't try to cast again too early due to server/packet lag and get rejected by the server with a "This ability isn't ready yet" error.  For 150ms I found 30ms to be a reasonable buffer.
2.  The second line is the window in ms before each cast during which nampower will delay cast attempts to send them to the server at the perfect time.  So 300 would mean if you cast anytime in the 300ms window before your next optimal cast your cast will be sent at the idea time.  This means you don't have to spam cast as aggressively.  This feature will cause a small stutter because it is pausing your UI (I couldn't findSpellId a better way to do this but I'm sure one exists now with superwow) so if you don't like that set this to 0.

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
