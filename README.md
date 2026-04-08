# Conway's Game of Life in the Terminal

This project creates Conway's Game of Life using the ncurses library.

You can also use your terminal screen as a:
- [Cylinder](https://en.wikipedia.org/wiki/Cylinder)
- [Torus](https://en.wikipedia.org/wiki/Torus)
- [Mobius Strip](https://en.wikipedia.org/wiki/M%C3%B6bius_strip)
- [Kline Bottle](https://en.wikipedia.org/wiki/Klein_bottle)
- [Projective Plane](https://en.wikipedia.org/wiki/Projective_plane)

## Controls
- `s`: Start the game
- `p`: Pause the game
- `r`: Resume the game
- `+`: Speed up
- `-`: Slow down

## Requirements
The ncurses library
```bash
sudo apt install libncurses5-dev libncursesw5-dev
```
A C complier (cc, gcc etc.)

## Building and Running
```bash
make
./life spaceship
```

You can also do it without make if you want:

```bash
gcc -g life.c -o life -lncurses
```
