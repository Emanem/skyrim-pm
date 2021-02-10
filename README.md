# skyrim-pm
Simple *Skyrim SE* plugin manager for linux; this is to make it easy to extract _FOMOD_ data according to such [specification](https://fomod-docs.readthedocs.io/en/latest/tutorial.html).

## Status

Currently does basic unpacking of compressed mods and should install and manage dynamic choices within each archive. Does not support inter-packages/mods references. If not specifying option `-s` (or `--sse-data`) it will try to lookup the Skyrim SE `Data` directory (otherwise in case of failure will output files/directories into `./Data` hence in such cases you want to run _skyrim-pm_ from wihtin `Skyrim Special Edition` directory).

If you have an archive without _ModuleConfig.xml_ you can try running it with `-x` (`--data-ext`) option to try install the files based on their names: _*.esp/bsa/ini_ will go into the specified _Data_ directory,whilst files under _meshes_, _textures_, _sound_ and _interface_ will be copied with their own relative paths under respective subdirectories.

### Known issues

* Some archives take longer to decompress; this is a _libarchive_ issue, unforutnately the performance of such archive is bad (installing _SMIM_ took approximately 50 minutes on a _5950x_).
* _RAR_ archives are not properly supported, sometime those fail extracting for no real reason; this is another _libarchive_ [issue](https://github.com/libarchive/libarchive/issues/1490). As a workaround it's easier to de-compress and compress again in another format.

## How to build

Download the sources, then get _libxml2_ and _libarchive_, dev version (i.e. `sudo apt install libxml2-dev libarchive-dev`), then invoke `make`.

## How to run
```
Usage: ./skyrim-pm [options] <mod1.7z> <mod2.rar> <mod3...>
Executes skyrim-pm 0.1.3

-s,--sse-data d Use specified Skyrim SE Data directory (d). If not set, skyrim-pm
                will try to invoke 'locate' to find it and use the first entry
-x,--data-ext   Try to extract the archive no matter what even when ModuleConfig.xml
                can't be found. In this case all files which match a given criteria
                will be extracted and saved under the specified Data directory
-p,--plugins f  Use specified file (f) as 'Plugins.txt' file; this parameter will
                imply automatically modifying such file to enabling ESP files without
                having to modify and use the 'load order' in-game menu; usually this
                file would be located under:
                <Local Settings/Application Data/Skyrim Special Edition/Plugins.txt>
--auto-plugins  Automatically find 'Plugins.txt' file and if found behaves as if option
                -p (or --plugins) got set to same file name (default disabled)
--log           Print log on std::cerr (default not set)
--xml-debug     Print xml debug info for ModuleConfig.xml
--no-colors     Do not display terminal colors/styles
```

### Run examples
If you're feeling somehow optimistic, you can run
```
./skyrim-pm -x --auto-plugins <mod1.7z> <mod2.zip> ...
```
This will optmistically try to install everything automatically. Setting the `--log` option would help out understanding potential issues in more details.

If instead you want to _preview_ actions, then one could redirect both _Data_ and _Plugins.txt_ to loca directory/file, adding options `-s ./Data` and `-p plugin.txt`, thus
```
./skyrim-pm -s ./Data -p plugins.txt -x <mod1.7z> <mod2.zip> ...
```

## F.A.Q.

1. *Why did you write this?* Wanted to understand and experiment _FOMOD_ format.
2. *What file formats are supported?* All archive (7z, tar, rar, zip, ...) as long as are supported by [libarchive](https://www.libarchive.org/); archives have to be compliand with _FOMOD_ format (i.e. containing an xml called _ModuleConfig.xml_ with detailed instruction on how to manage files).
3. *Extracting large mod files (i.e. SMIM) takes ages. Why?* This application uses _libarchive_ to look into archives; whilst it's a very easy to use API and supports almost _all_ formats, it only allows sequential scans, hence extracting files becomes slower in some cases because the same archive needs to be traversed multiple times.
4. *How can I see more details of what *skyrim-pm* is doing?* Just specify the `--log` option.
5. *I think feature *x* would be cool. How can I get it?* Simply open a bug on this github repository.
6. *I want to install a mod, but it doesn't come with *FOMOD* format. How can I do it right?* In this case you won't be able to use *skyrim-pm*, you'll have to manually extract it. Just remember to convert **all** the files inside `meshes` and `texture` subdirectories to lower case (and replace existing files possibly). Alternatively you can run with option `-x` (or `--data-ext`) but be aware that _shyrim-pm_ will try its best to install files (recommended to also run with `--log` option).
7. *Looks like data extraction is slow and I have one CPU core pegged to 100%. Why?* This is due to libarchive operational execution.
8. *I have installed *Skyrim SE* but nor *Data* nor *Plugins.txt* can be automatically found. Any suggestion?* Run `sudo updatedb`, this will refresh the `locate` cache (*skyrim-pm* uses such program to find ou these files).

## Todo

- [ ] Investigate if there's a better way to improve libarchive usage/performance
- [x] Add option to modify *Plugins.txt* to automatically add and enable *esp* files
- [x] Support _raw data_ extraction for archives without _ModuleConfig.xml_
- [x] Ensure option `-s` is properly managed
- [x] Automatically `locate` *Skyrim SE* `Data` directory
- [ ] ...
