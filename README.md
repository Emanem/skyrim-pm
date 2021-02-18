# skyrim-pm
Simple *Skyrim SE* plugin manager for linux; this is to make it easy to extract _FOMOD_ data according to such [specification](https://fomod-docs.readthedocs.io/en/latest/tutorial.html).

## Status

Currently does basic unpacking of compressed mods and should install and manage dynamic choices within each archive. Does support inter-packages/mods references when specifying the `-o` (or `--override`) option.

If not specifying option `-s` (or `--sse-data`) it will try to lookup the Skyrim SE `Data` directory (otherwise in case of failure will output files/directories into `./Data` hence in such cases you want to run _skyrim-pm_ from wihtin `Skyrim Special Edition` directory).

If you have an archive without _ModuleConfig.xml_ you can try running it with `-x` (`--data-ext`) option to try install the files based on their names: _*.esp/bsa/ini_ will go into the specified _Data_ directory,whilst files under _meshes_, _textures_, _sound_ and _interface_ will be copied with their own relative paths under respective subdirectories.

### Known issues

* Some archives take longer to decompress; this is a _libarchive_ issue, unforutnately the performance of such archive is bad (installing _SMIM_ took approximately 50 minutes on a _5950x_).
* _RAR_ archives are not properly supported, sometime those fail extracting for no real reason; this is another _libarchive_ [issue](https://github.com/libarchive/libarchive/issues/1490). As a workaround it's easier to de-compress and compress again in another format.

## How to build

Download the sources, then get _libxml2_ and _libarchive_, dev version (i.e. `sudo apt install libxml2-dev libarchive-dev`), then invoke `make` (or `make release` for optimized version).

## How to run
```
Usage: ./skyrim-pm [options] <mod1.7z> <mod2.rar> <mod3...>
Executes skyrim-pm 0.2.0

Basic options (files will be overwritten in Data directory)

-s,--sse-data d   Use specified Skyrim SE Data directory (d). If not set, skyrim-pm
                  will try to invoke 'locate' to find it and use the first entry
-x,--data-ext     Try to extract the archive no matter what even when ModuleConfig.xml
                  can't be found. In this case all files which match a given criteria
                  will be extracted and saved under the specified Data directory
-p,--plugins f    Use specified file (f) as 'Plugins.txt' file; this parameter will
                  imply automatically modifying such file to enabling ESP files without
                  having to modify and use the 'load order' in-game menu; usually this
                  file would be located under:
                  <Local Settings/Application Data/Skyrim Special Edition/Plugins.txt>
--auto-plugins    Automatically find 'Plugins.txt' file and if found behaves as if option
                  -p (or --plugins) got set to same file name (default disabled)

Override options (files will be saved in override directory and only symlinks will be
written in Data   directory - furthermore the file Data/skyrim-pm-fso.xml will be used
to control such   overrides over time)

-o,--override d   Do not write files into Skyrim SE 'Data' directory but in directory 'd'
                  skyrim-pm will instead write symlinks under 'Data' directories and will
                  write a new 'xml' file under 'Data' directory to manage such symlinks.
                  If an existing file is present under 'Data' it will be overwritten by
                  the symlinks and won't be recoverable
-l,--list-ovd     Lists all overrides/installed plugins
--list-replace    Lists all the overridden files which have been replaced by successive
                  plugins (i.e. when plugins/mods potentially have conflicted during setup
                  process)
--list-verify     Checks all the symlinks in the override config file are still present
                  under Data and also that all the files in such config are still available
                  on the filesystem
-r,--list-remove  Try to remove the listed plugins, restoring the previous overridden symlinks
                  when applicable

Misc/Debug options

-h,--help         Print this text and exits
--log             Print log on std::cerr (default not set)
--xml-debug       Print xml debug info for ModuleConfig.xml
--no-colors       Do not display terminal colors/styles
```

### Run examples
Some run examples, without and with _override_.

#### Basic (Without overrides)
If you're feeling somehow optimistic, you can run
```
./skyrim-pm -x --auto-plugins <mod1.7z> <mod2.zip> ...
```
This will try to install everything automatically. Setting the `--log` option would help out understanding potential issues in more details.
In this case the whole content will be installed under *Data* directory and mods with conflicting files will overwrite each other - this will be a non reversible operation.

#### Advanced (with overrides)
The following examples will all copy the real mod files somwhere specified by `-o` and then always create _symlinks_ inside _Data_ directory. Furthermore an XML file used by _skyrim-pm_ (named _skyrim-pm-fso.xml_) will be created under _Data_ and will be used to manage such _symlinks_.

The advantages of this apporach are that you will be able to list when mod replace each others (`-l`), verify integrity of installed mods (`--list-verify`) and also remove mods (`-r`) and automatically have the symlinks fall back to previous overriden mods (if any). 
```
./skyrim-pm -x --auto-plugins -o /path/to/real/files <mod1.7z> <mod2.zip> <mod3.xz>
```
This will install/extract mods in specified order, virtually overriding `<mod1.7z>` content with `<mod2.zip>`, and `<mod2.zip>` with `<mod3.xz>`; if the same mod file is present in all 3, `<mod3.xz>` version will be kept, and so on.
Then invoking
```
./skyrim-pm -o /path/to/real/files -l
```
will list the installed mods. Following with
```
./skyrim-pm -o /path/to/real/files --list-replace
```
will show what are the replace/overridden files, and running:
```
./skyrim-pm -o /path/to/real/files -r <mod2.zip>
```
will remove all the files from `<mod2.zip>` and also restore the symlinks so that if any file from `<mod2.zip>` was overriding a file from `<mod1.7z>`, the one from `<mod1.7z>` will be now restored as symlink.

## F.A.Q.

1. *Why did you write this?* Wanted to understand and experiment _FOMOD_ format.
2. *What file formats are supported?* All archive (7z, tar, rar, zip, ...) as long as are supported by [libarchive](https://www.libarchive.org/); archives have to be compliant with _FOMOD_ format (i.e. containing an xml called _ModuleConfig.xml_ with detailed instruction on how to manage files).
3. *Extracting large mod files (i.e. SMIM) takes ages. Why?* This application uses _libarchive_ to look into archives; whilst it's a very easy to use API and supports almost _all_ formats, it only allows sequential scans, hence extracting files becomes slower in some cases because the same archive needs to be traversed multiple times.
4. *How can I see more details of what *skyrim-pm* is doing?* Just specify the `--log` option.
5. *I think feature *x* would be cool. How can I get it?* Simply open a bug on this github repository.
6. *I want to install a mod, but it doesn't come with *FOMOD* format. How can I do it right?* You can run with option `-x` (or `--data-ext`) but be aware that _skrim-pm_ will try its best to install files (recommended to also run with `--log` option).
7. *Looks like data extraction is slow and I have one CPU core pegged to 100%. Why?* This is due to libarchive operational execution.
8. *I have installed *Skyrim SE* but nor *Data* nor *Plugins.txt* can be automatically found. Any suggestion?* Run `sudo updatedb`, this will refresh the `locate` cache (*skyrim-pm* uses such program to find ou these files).

## Todo

- [ ] Investigate if there's a better way to improve libarchive usage/performance
- [x] Add support for overlay/override Data directory and automated symlink fallback on plugin removal
- [x] Add option to modify *Plugins.txt* to automatically add and enable *esp* files
- [x] Support _raw data_ extraction for archives without _ModuleConfig.xml_
- [x] Ensure option `-s` is properly managed
- [x] Automatically `locate` *Skyrim SE* `Data` directory
- [ ] ...
