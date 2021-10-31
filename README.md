# minesweeper with selectable "sweeper size"

First things first: Thanks to Henri Sarasvirta for the idea.

Second: Thanks to @rdiachenko for a good looking base to implement the idea on top of.
Check his branches README for extra info I decided to cut from here.

## What's "Sweeper size"?

Normal minesweeper square checks the 8 neighboring tiles, or simply the tiles in a 3 by 3 square around
the tile itself. By increasing the square to 5 by 5 tiles, the game gets a lot harder, and my intuition
says the probability of having to guess goes down. Win-Win.

## Required dependencies
* SDL2
* SDL2_image
```
# E.g.: installation for Fedora 24

$ sudo dnf install SDL2 SDL2-devel SDL2_image SDL2_image-devel
```

## Build and install
```
$ mkdir build && cd build
$ cmake ../
$ make
$ make install

# after completion minesweeper should be installed in minesweeper/release folder
```

## Launch with 5x5 configuration
```
# default configuration

$ cat resources/5x5.conf
mines=40
field_rows=16
field_cols=30
sweeper_size=5
sprite_img=resources/classic.png
sprite_txt=resources/classic.txt

# launch
$ ./minesweeper resources/5x5.conf
```
![](https://raw.githubusercontent.com/shiona/minesweeper/master/screenshots/5x5.png)

