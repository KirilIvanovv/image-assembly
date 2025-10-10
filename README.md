# Imageassembly

Format for assembling images. It's used to create partially rendered 3D images
that can be textured later. 

A library is available for C and Go languages, as well as a CLI tool for working
with the format, written in C.

## Why even?

Consider that you have a 3D model and you want users to be able to submit images
that will be used to texture said 3D model. A naive approach would be to submit
the 3D model and texture image to a graphics package for raytracing for every
user submission.

![example1](docs/example1.jpg)

Rendering this image took 4.34 seconds using Tramway SDK. This is far too
inefficient to be done at scale. A far better approach would be to precompute
the results of raytracing geometry intersections and lighting computations, then
perform only texturing for every user submission.

![example2](docs/example2.jpg)

Assembling each image took 197 milliseconds on average. This is a significantly
improved result.

## How it work?

Unlike regular image formats, which store the value of each pixel as a RGB color
value or a palette index, this format stores it as a sequence of bytecode
instructions, which when executed produce the final RGB color value of the
pixel.

This format supports storing multiple frames in a single image, which is perfect
for creating animated .gifs or short animations for tiktok.

### Example

Let's look at the bytecode at pixel 320x 240y in the above image. It's right in
the middle of the image.
```
2734616 LOAD 01, 93 93 93 ff  ; load #939393 into register 1
2734622 SMPL 02, 01 0144 0358 ; sample input img 1 @ 144x 358y into r2
2734628 MULT 01, 02           ; multiply r1 & r2 and store into r1
2734630 ADD  00, 01           ; add r1 to r0 (output register)
2734632 LOAD 01, 00 00 00 ff  ; load #000000 into r1
2734638 ADD  00, 01           ; add r1 to r0
2734640 RET                   ; return r0 as final color
```
The virtual machine has 4 registers, each holding a 32-bit RGBA value. Before
executing the code of each pixel, all of the registers are zeroed out. The value
that is stored in register 0 at the end of the program is stored in the output
buffer.

## What is using?

[Tramway SDK](https://github.com/racenis/tram-sdk) graphics package has an
extension that can save the output of its raytracing renderer as an imageassmbly
file.

[Greeting Card](https://kaimlatechnology.com/saas/) web service currently is
using imageassembly library to provide image generation as an API.