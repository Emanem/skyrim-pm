# skyrim-pm
Simple *Skyrim SE* plugin manager for linux; this is to make it easy to extract _FOMOD_ data according to such [specification](https://fomod-docs.readthedocs.io/en/latest/tutorial.html).

## Status

Currently does basic unpacking of compressed mods and should install and manage dynamic choices within each archive. Does not support inter-packages/mods references. If not specifying option `-s` (or `--sse-data`) it will try to lookup the Skyrim SE `Data` directory (otherwise in case of failure will output files/directories into `./Data` hence in such cases you want to run _skyrim-pm_ from wihtin `Skyrim Special Edition` directory).

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

## F.A.Q.

1. *Why did you write this?* Wanted to understand and experiment _FOMOD_ format.
2. *What file formats are supported?* All archive (7z, tar, rar, zip, ...) as long as are supported by [libarchive](https://www.libarchive.org/); archives have to be compliand with _FOMOD_ format (i.e. containing an xml called _ModuleConfig.xml_ with detailed instruction on how to manage files).
3. *Extracting large mod files (i.e. SMIM) takes ages. Why?* This application uses _libarchive_ to look into archives; whilst it's a very easy to use API and supports almost _all_ formats, it only allows sequential scans, hence extracting files becomes slower in some cases because the same archive needs to be traversed multiple times.
4. *How can I see more details of what *skyrim-pm* is doing?* Just specify the `--log` option.
5. *I think feature *x* would be cool. How can I get it?* Simply open a bug on this github repository.
6. *I want to install a mod, but it doesn't come with *FOMOD* format. How can I do it right?* In this case you won't be able to use *skyrim-pm*, you'll have to manually extract it. Just remember to convert **all** the files inside `meshes` and `texture` subdirectories to lower case (and replace existing files possibly).
7. *Looks like data extraction is slow and I have one CPU core pegged to 100%. Why?* This is due to libarchive operational execution.

## Todo

- [ ] Investigate if there's a better way to improve libarchive usage/performance
- [x] Ensure option `-s` is properly managed
- [x] Automatically `locate` *Skyrim SE* `Data` directory
- [ ] ...
