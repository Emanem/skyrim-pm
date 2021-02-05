# skyrim-pm
Simple *Skyrim SE* plugin manager for linux; this is to make it easy to extract _FOMOD_ data according to such [specification](https://fomod-docs.readthedocs.io/en/latest/tutorial.html).

## Status

Currently does basic unpacking of compressed mods and should install and manage dynamic choices within each archive. Does not support inter-packages/mods references. As of now it doesn't lookup the Skyrim `Data` directory, and will output files/directories into `./Data`, hence you want to run it from main *Skyrim SE* install directory.

## How to build

Download the sources, then get _libxml2_ and _libarchive_, dev version (i.e. `sudo apt install libxml2-dev libarchive-dev`), then invoke `make`.

## How to run
```
Usage: ./skyrim-pm [options] <mod1.7z> <mod2.rar> <mod3...>
Executes skyrim-pm 0.1.1

-s,--sse-data   Use specified Skyrim SE data directory. If not set, skyrim-pm
                will try to invoke 'locate' to find it and use the first entry
--log           Print log on std::cerr (default not set)
--xml-debug     Print xml debug info for ModuleConfig.xml
--no-colors     Do not display terminal colors/styles
```

## Todo

- [ ] Ensure option `-s` is properly managed
- [ ] Automatically `locate` *Skyrim SE* `Data` directory
- [ ] ...
