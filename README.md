## Image Defogging Utility ##

A small program that removes fog and haze from images.

Uses a modified version of the algorithm outlined in [this paper](http://www.fujitsu.com/downloads/MAG/vol50-1/paper10.pdf); at some point, I'll get around to writing up exactly what this code does.

### Building ###

	gcc -o defog src/defog.c `pkg-config --libs --cflags opencv` -std=c99 -lm

### Running ###

	./defog IMAGE_FILE

where `IMAGE_FILE` is a color (RBG) image; the algorithm won't work on grayscale images.

### License ###

The algorithm used was designed by Zhiming Tan, Xianghui Bai, Bingrong Wang, and Akihiro Higashi.

All code is made available under the following license:

	The MIT License (MIT)

	Copyright (c) 2014-2015 David Pearson

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.