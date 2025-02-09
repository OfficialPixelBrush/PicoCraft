# PicoCraft
 A bare-bones Minecraft Beta 1.7.3 Server that can run on a Raspberry Pi Pico W.
 
 Think of it more like Minecraft Classic but it needs a Beta 1.7.3 Client.

# Features
- Multiplayer
- WiFi or AP Modes
- 16x16x16 world size
- Flat world generation
- Colored chat messages
- 9 blocks to choose from
- Block placing and breaking

# Building
Just use the Arduino IDE and flash it on as you would with any other Arduino Project.

Remember to set your WiFis SSID and password so the Pi can connect.

Alternatively you can enable AP Mode, which lets the Pi make its own network (but this limits you to only 4 Clients).

# Limitations
## Small World Size
The World size of 16x16x16 isn't a limitation of the Pis memory,
I'm certain it could handle many more chunks in there.
The main issue is, due to sending uncompressed chunk data, the Pi can't
keep up with much more than a few Bytes at a time. Even increasing the 
world size to 16x24x16 already results in severe corruptions.

A way to address this may be to send a chunk over several packets, possibly as vertical strips.

## Everything else
Other missing features, such as
- Client-side players
- PvP
- Tools
- Infinite worlds
- *etc.*

are missing, simply due to not being implemented or not being planned. 
This project is moreso meant as a little weekend side-project to pass the time.

# Hacks
## Not storing data we don't need
For the purposes of this project I've decided to not store any Block Metadata/Damage Values or lighting data.
All that is stored are the block IDs of the current world.

## Sending chunks without zlib
Instead of using any of the zlib implementations out there, which I tried to use,
I found I was too starved of memory to make any of them work.
As a result, I began digging into the specs of [zlib](https://www.rfc-editor.org/rfc/rfc1950) and [deflate](https://www.rfc-editor.org/rfc/rfc1951).

While zlib doesn't support uncompressed data, deflate does,
and since zlib is just a thin wrapper around deflated data,
it was relatively trivial to send uncompressed chunk data.

## Slightly modified Adler32
At the end of a zlib packet is a little [Adler-32](https://en.wikipedia.org/wiki/Adler-32) checksum, to ensure all data was received correctly.
While this is usually great, its problematic when one is intentionally not including data that the client still expects.
The solution was to sneakily pretend that data is there anyways and make it part of the checksum too.


# References
- [wiki.vg (Now part of the Minecraft Wiki)](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Protocol?oldid=27697)
- [RFC 1950 (ZLIB Compressed Data Format Specification)](https://www.rfc-editor.org/rfc/rfc1950)
- [RFC 1951 (DEFLATE Compressed Data Format Specification)](https://www.rfc-editor.org/rfc/rfc1951)
- [Adler-32 - Wikipedia](https://en.wikipedia.org/wiki/Adler-32)