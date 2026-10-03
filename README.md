# Lyla - Overly-ambitious music player



https://github.com/user-attachments/assets/87132c79-4ad7-4bd0-891a-5f234814d6b6

Music by [Rigel Theatre](https://www.rigeltheatre.com/)

Lyla is a music player with built in visualization inspired by [tsoding](https://github.com/tsoding/) and since
the visualization is on start of the stream and not yet open source I'm going to make my very own with my own idea.

## Requirements

| Component |   Minimum                        | Recommended                                                                      |
|-----------|----------------------------------|----------------------------------------------------------------------------------|
|    OS     |   Windows, Linux                 |   Windows 30K                                                                    |
|    CPU    |   1.0 Ghz                        |   6769.0Thz                                                                      |
|    RAM    |   512 MB                         |   10000 TB                                                                       |
|    GPU    |   256 MB                         |   50000.0 TB                                                                     |
|  Storage  |   80mb                           |   10TB                                                                           |
|  Graphics | Hardware accelerated OpenGL (3.3)| OpenGL (6969)                                                                    |
| Sound Card|   Any                            | Sound Card That Makes The Music Sound Like A Live Orchestra Even Though It's Not |

*The requirement might change over the course of development

## Keybinds

| Key   | Behavior                            |
|-------|-------------------------------------|
| `     | open playlist                       |
| enter | confirm selection / next song       |
| s     | shuffle mode                        |
| t     | toggle track info                   |
| space | play/pause                          |
| r     | reset progress                      |
| f     | fullscreen                          |
| c     | clear playlist                      |
| d     | force detach the (yo) ball          |
| f1    | toggle background thumbnail panning |
| f3    | toggle input (wallpaper mode)       |

There are two mode in windows, a wallpaper mode and normal mode, in wallpaper mode all the control is disabled until you press
specific key to activate it, the wallpaper mode only available on windows for now in system tray.

## Build

Requirement: meson.build, cmake, ninja, make

There is no easy way to build since I use custom Raylib dependency, and maybe more custom thing
that I made it private due to my life circumstance (feel free to deduce why).

For now you can use your classic Raylib source with meson.build that has meson.options in there

## Contributions

To contribute or file a request feel free to email me.

All of the idea or tasks is in the `tasks` folder, I strictly deny the 
use of Github Issue and Pull Request for now due to also my life circumstance (feel free to deduce why).

## LICENSE

This project is licensed under **PolyForm Noncommercial License 1.0.0**
