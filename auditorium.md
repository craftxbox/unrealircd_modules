# third/auditorium
This module adds the chmode +u, or "Auditorium Mode"  
When set on a channel, Users can only see users who have the chmodes specified in +u  
This will also be bypassed for any IRCops, they will be able to see anyone and send messages to anyone.  
The IRCop will however need to SAMODE to match +u to actually *see* messages  
if you have third/ojoin however, this is handled automatically and any opers OJOINing with +Y will be able to see everything.  
  
Because of some [internal restrictions](https://github.com/unrealircd/unrealircd/blob/4504adf/src/channel.c#L1290) however, anyone with +vhoaq will be able to be seen regardless of +u setting.  
People with +hoaq will also be able to see anyone regardless of +u setting.  
  
# Instructions
Place [auditorium.c](/auditorium.c) into src/modules/third of your unrealircd source.  
(OPTIONAL) Install third/ojoin from the UnrealIRCD module manager aswell
`$ make && make install`  
Set +u on any channel and then /cycle everyone. Requires one of +aq to actually set.
  
# Why this exists
I was building an automated relay, where the bot would end up re-emitting the message you sent.  
This would look super messy if multiple people were just chatting on the same server, they'd see everyone's message twice.  
Having auditorium would cut down on this and make users only see their own messages twice instead.  
However, when i poked at the v2.1.1 third/auditorium module, it seemed unneccesarily restrictive,  
being FIXED to only +oaq able to see, So i decided to modify this to be more flexible.
I was also inspired by third/ojoin so decided to integrate them together.
  
# License

Credit to `Gottem`, who wrote the [original third/auditorium](https://gottem.nl/unreal/man/auditorium)

[AGPLv3](https://gnu.org/licenses/agpl-3.0.html)
