# Hammond Novachord LV2 Plugin
By Julian Reid

## Foreword
This is a small lv2 synthesizer plugin that I developed for a university project in 2016. The code is very messy as I was in a rush while developing it. There will be blocks of code that are no longer part of the project, unsafe C code, many TODOs and likely some bugs. Much of the implementation is very inefficient and would benefit from a lot of optimization and wide instruction set usage. I will probably come back and tidy up the code in the future however I am unlikely to add any more features.

## About
This plugin is an attempt to recreate the sound of the Novachord synthesizer developed by Hammond around 1939. Unfortunatly it probably is not very accurate as I did not have any access to a Novachord or high quality samples of a Novachord while developing it. My target sound was based on low quality recordings, circuit diagrams, the instrument manual and a traced waveform. Although not quite the signature Novachord sound, in my opinion it does still produce some quite cool, retro sounding synth voices!

## Usage
The controls are directly analogous to the original Novachord which means some things like the "attack" parameter don't mean exactly what you would expect them to. Attack in this case also influences the sustain volume from settings 1 to 4. As such the sustain pedals will not function unless you have the "attack" setting at at least 2. I’d advise just playing around with each of the knobs to figure out how they influence the sound but there’s also a few Novachord manuals online that will give you an idea of what they do.

I think you get the nicest sounds by turning the full tone and deep tone down and raising one or more resonators to a high value. I think that emphasizes the unique bits of the waveform that the Novachord generates.

## Installation
- run `waf configure`
- If you are missing any dependencies or build tools (gcc, pkg-config, gtk2, cairo, lv2) then install them and run `waf configure` again
- run `waf`
- The plugin will then be built into the "build" folder, move the "novachord.lv2" folder within "build" into your lv2 plugin folder of choice ("usr/lib/lv2" is searched by most lv2 hosts)

## Additional Credits
This repository also contains some files developed by others:
- waf - A build script by Thomas Nagy, distributed with lv2 examples
- threeband.c - A 3 band equalizer implementation by Neil C of Etanza Systems
All retain their respective licences at the top of the source files
