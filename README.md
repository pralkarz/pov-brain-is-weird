# pov: brain is weird

simple toy to showcase the persistence of vision phenomenon discussed in [this video](https://www.youtube.com/watch?v=TdTMeNXCnTs).

try starting the below demo. when it's running, you will seemingly notice lines flickering on the screen, but they disappear as soon as you pause or take a screenshot.

https://github.com/ziebam/pov-brain-is-weird/assets/31964869/b8616c97-452e-4ef5-aabc-a53f59d0165d

# build

you can build this project locally on Windows by default and on Linux with the `-platform` flag (sorry macOS folks).

requirements: `gcc` and `ar` available in the PATH.

steps:

1. clone the repo
2. `gcc ./nob.c -o nob`
3. `./nob` or `./nob -platform windows` for Windows or `./nob -platform linux` for Linux
4. `./build/pov-brain-is-weird`

keybindings:

- arrows (<kbd>←</kbd><kbd>↓</kbd><kbd>↑</kbd><kbd>→</kbd>) to choose a simulation
- <kbd>Enter</kbd> to select the simulation
- <kbd>p</kbd> to pause/unpause
- <kbd>ESC</kbd> to quit the simulation and go back to menu

# credits

`nob` -- amazing, minimalist build system by [Tsoding](https://github.com/tsoding), license included in `nob.h`

`raylib` -- amazing, minimalist game-making library, [license](./raylib/raylib-5.0/LICENSE) vendored in the repository

window icon -- taken from [Tsoding](https://github.com/tsoding)'s stream/Discord server with his persmission: [click!](https://twitchemotes.com/channels/110240192)

inspiration -- [this video](https://www.youtube.com/watch?v=TdTMeNXCnTs) and its unironically awesome comment section
