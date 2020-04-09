# Tossup emulator notes


## Background

In 2002, I created a series of emulators for the Nintendo Ball Game and Watch device. While there was already a Ball emulator available I wanted to create one of my own and achieve the following goals:

* Clean graphics. 
* Smooth. At the time many of the Game and Watch were written in Visual Basic 6. 
* Accurate

In 2020, I was approach to tweak the HTML5 version of the emulator to work in a giant recreation to celebrate the 40th anniversary of Ball's launch. 


## Graphics

Back in 2002, I digitized the graphics by scanning the device on a flatbed scanner and tracing the outline of the raster image using CorelDRAW!. This approach was heavily influenced by the technology I had to hand. I was an early adopter of digital photography, but my camera at the time was 2MP so taking a photo of it was going to be quite pixelated and not ideal for tracing by hand. Also, vector drawing tools were not freely (as in beer) available so I used a somewhat old version of CorelDRAW. Th challenge at the time was to take the outlines from CorelDRAW and turn them into something I could use in Win32 API calls. This was quite fiddly and the best I could do was export the vector outlines to postscript and write a separate program to extract the outlines. It worked but had some drawbacks:

* Lack of precision - the postscript file used whole number of points as units. To integer number of points and
* Adhoc - postscript is quite complicated to parse and I did the minimum amount of hacking to get things working.
* No meta data -

In 2020, things have improved somewhat. I was able to take the original .cdr file and convert it to SVG. 

* Better precision - My converted file uses coordinates with 8 digits of decimal precision
* Cleaner - SVG is a standard format and reading XML is much easier than postscript
* Metadata - Inkscape allows the user to label objects. This allows the paths to be labelled in the UI and exported easily.


## Sound


## Timings

The most complicated part of creating the emulator was figuring out the precise movement of the game. Toss up is an extremely simple game and balls move in a predictable way at regular intervals. The challenge is to figure out the duration of these regular intervals are and how they speed up as the game progresses. One way to do this is to record the beeps from a long game session and pick out the timings from the recording. The downside of this approach is you need to be competent player to get a long enough recording. Ideally one would like to know what happens when the score reaches 9999 and wraps over. For 'Game A' that can take over six hours. Toss up is a pretty boring game and I'm also not that good it. There was no way I was going to be able to play the game for that long so I was left with a decision -- I could guess what happens (afterall real people are never going to play the emulator for that long) or I could build a device to play the game. Being obessed with accuracy I chose the latter option. 