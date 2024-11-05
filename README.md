# Pepo v2.0.0 Changes

Added spell queuing, automatic retry on error, and quickcasting with lots of customization.  

Some other key improvements over Namreeb's version:
- Using a buffer to avoid server rejections from casting too quickly (namreeb's uses 0 buffer).  See 'Why do I need a buffer?' below for more info.
- Using high_resolution_clock instead of GetTickCount for fasting timing on when to start casts
- Fix broken cast animations when casting spells back to back

### Compatability with other addons
Queuing can cause issues with some addons that also manage spell casting.  Quickheal/Healbot/Quiver do not work well with queuing.  Check github issues for other potential incompatibilities.
If someone rewrites these addons to use guids from superwow that would likely fix all issues.

Additionally, if you use pfui mouseover macros there is a timing issue that can occur causing it to target yourself instead of your mouseover target.

HIGHLY recommend changing line 80 in pfui/modules/mouseover.lua from 

`if SpellIsTargeting() then SpellTargetUnit(unitstr or "player") end`

to 
`SpellTargetUnit(unitstr or "player")`

### Configuration

#### Configure with addon
There is a companion addon to make it easy to check/change the settings in game.  You can download it [here](https://github.com/pepopo978/nampowersettings).

#### Manual Configuration
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
- `NP_OptimizeBufferUsingPacketTimings` - Whether to attempt to optimize your buffer using your latency and server packet timings (more explanation below).  0 to disable, 1 to enable. Default is 0.


- `NP_ChannelLatencyReductionPercentage` - The percentage of your latency to subtract from the end of a channel duration to optimize cast time while hopefully not losing any ticks (more explanation below). Default is 75.

### Custom Lua Functions

#### QueueSpellByName(spellName)
Will force queue a spell regardless of the appropriate queue window.  If no spell is currently being cast it will be cast immediately.
For example can make a macro with 
```
/run QueueSpellByName("Frostbolt");QueueSpellByName("Frostbolt")
```
to cast 2 frostbolts in a row.  Currently, can only queue 1 GCD spell at a time and 5 non gcd spells.  This means you can't do 3 frostbolts in a row with one macro.

#### CastSpellByNameNoQueue(spellName)
Will force a spell cast to never queue even if your settings would normally queue.  Can be used to fix addons that don't work with queued spells.

#### QueueScript(script)
Queues any arbitrary script using the same logic as a regular spell using NP_SpellQueueWindowMs as the window.  The script will run BEFORE any other queued spells.  If no spell is being cast and you are not on the gcd the script will be run immediately.

Convert slash commands from other addons like `/equip` to their function form `SlashCmdList.EQUIP` to use them inside QueueScript.

For example, you can equip a libram before casting a queued heal using
```
/run QueueScript('SlashCmdList.EQUIP("Libram of +heal")')
```

### SPELL_QUEUE_EVENT
I've added a new event you can register in game to get updates when spells are added and popped from the queue.

The event is `SPELL_QUEUE_EVENT` and has 2 arguments:
`(int eventCode, int spellId)`

Possible Event codes:
```
 ON_SWING_QUEUED = 0
 ON_SWING_QUEUE_POPPED = 1
 NORMAL_QUEUED = 2
 NORMAL_QUEUE_POPPED = 3
 NON_GCD_QUEUED = 4
 NON_GCD_QUEUE_POPPED = 5
```

Example from NampowerSettings:
```
local ON_SWING_QUEUED = 0
local ON_SWING_QUEUE_POPPED = 1
local NORMAL_QUEUED = 2
local NORMAL_QUEUE_POPPED = 3
local NON_GCD_QUEUED = 4
local NON_GCD_QUEUE_POPPED = 5

local function spellQueueEvent(eventCode, spellId)
	if eventCode == NORMAL_QUEUED or eventCode == NON_GCD_QUEUED then
		local _, _, texture = SpellInfo(spellId) -- superwow function
		Nampower.queued_spell.texture:SetTexture(texture)
		Nampower.queued_spell:Show()
	elseif eventCode == NORMAL_QUEUE_POPPED or eventCode == NON_GCD_QUEUE_POPPED then
		Nampower.queued_spell:Hide()
	end
end

NampowerSettings:RegisterEvent("SPELL_QUEUE_EVENT", spellQueueEvent)
```

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

There are 3 separate queues for the following types of spells: GCD(max size:1), non GCD(max size:5), and on-hit(max size:1).

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

#### GCD Spells
Only one gcd spell can be queued at a time.  Pressing a new gcd spell will replace any existing queued gcd spell.

#### Non GCD Spells
Non gcd spells have special handling.  You can queue up to 5 non gcd spells, 
and they will execute in the order queued with `NP_NonGcdBufferTimeMs` delay after each of them to help avoid server rejection.  
The non gcd queue always has priority over queued normal spells.  
You can only queue a given spellId once in the non gcd queue, any subsequent attempts will just replace the existing entry in the queue.

`NP_ReplaceMatchingNonGcdCategory` will cause non gcd spells with the same StartRecoveryCategory to replace each other in the queue.  
The vast majority of spells not on the gcd have category '0' so enabling this setting will cause them to all replace each other.  
One notable exception is shaman totems on TWoW that were changed to have separate categories according to their elements.

This can be useful if you want to change your mind about the non gcd spell you have queued.  For example, if you queue a mana potion and decide you want to use LIP instead last minute.
However, this will also prevent using certain cooldowns together with trinkets, such as Combustion followed by Mind Quickening Gem.  Decide what works best for you.

#### On hit Spells
Only one on hit spell can be queued at a time.  Pressing a new on hit spell will replace any existing queued on hit spell.  
On hit spells have no effect on the gcd or non gcd queues as they are handled entirely separately and are resolved by your auto attack.

#### Channeling Spells
Channeling spells function differently than other spells in that the channel in the client actually begins when you receive 
the CHANNEL_START packet from the server.  This means the client channel is happening 1/2 your latency after the server channel 
and that server tick delay is already included in the cast, whereas regular spells are the other way around (the client is ahead of the server).  

From my testing it seems that you can usually subtract your full latency from the end of the channel duration without losing
any ticks.  Since your latency can vary it is safer to do a percentage of your latency instead to minimize the chance of 
having a tick cut off.  This is controlled by the cvar `NP_ChannelLatencyReductionPercentage` which defaults to 75.

#### NP_OptimizeBufferUsingPacketTimings
This feature will attempt to optimize your buffer on individual casts using your latency and server packet timings.  
After you begin to cast a spell you will get a cast result packet back from the server letting you know if the cast was successful.
The time between when you send your start cast packet and when you receive the cast result packet consists of:
- The time it takes for your packet to reach the server
- The time it takes for the server to process the packet (see Why do I need a buffer?)
- The time it takes for the server to send the result packet back to you
- Other delays I'm not sure about like the time it takes for the client to process packets from the server due to being single threaded

If we take this 'Spell Response Time' and subtract your regular latency from it, we should be able to get a rough idea of the time it
took for the server to process the cast.  If that time was less than your current default buffer we can use that time as the new buffer
for the next cast only.  In theory if it is more than your current buffer we should also use it, but in 
practice it seems to regularly be way larger than expected and using the default buffer doesn't result in an error.

Due to this delay varying wildly in testing I'm unsure how reliable this technique is.  It needs more testing
and a better understanding of all the factors introducing delay.  It is disabled by default for now.

# Pepo v1.0.0 Changes

Now looks for a nampower.cfg file in the same directory with two lines:

1.  The first line should contain the "buffer" time between each cast.  This is the amount of time to delay casts to ensure you don't try to cast again too early due to server/packet lag and get rejected by the server with a "This ability isn't ready yet" error.  For 150ms I found 30ms to be a reasonable buffer.
2.  The second line is the window in ms before each cast during which nampower will delay cast attempts to send them to the server at the perfect time.  So 300 would mean if you cast anytime in the 300ms window before your next optimal cast your cast will be sent at the idea time.  This means you don't have to spam cast as aggressively.  This feature will cause a small stutter because it is pausing your UI (I couldn't findSpellId a better way to do this but I'm sure one exists now with superwow) so if you don't like that set this to 0.

# Namreeb readme

**Please consider donating if you use this tool. - Namreeb**

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=QFWZUEMC5N3SW)

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

## Using ##

If you use my launcher, known as [wowreeb](https://github.com/namreeb/wowreeb), you can add
the following line to a `<Realm>` block to tell the launcher to include this tool:

```xml
    <DLL Path="c:\path\to\nampower.dll" Method="Load" />
```

To launch with the built-in launcher, run loader.exe -p c:\path\to\wow.exe (or just loader.exe
with it inside the main wow folder)
