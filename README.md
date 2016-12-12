# skiaex

This sample demonstrates how to use Skia and HarfBuzz to produce .pdf-file from a given stdin.

HarfBuzz and its prerequisites
===

You have to install HarfBuzz if you don't have it on your machine.

For example, like this:

```
sudo apt-get install autoconf automake libtool pkg-config ragel gtk-doc-tools
git clone https://github.com/behdad/harfbuzz.git
cd harfbuzz
./autogen.sh
./configure
make && sudo make install
```

Build and run using gn
===

Create .gclient file in your working folder:

```
solutions = [
  { "name"        : "src",
    "url"         : "https://github.com/aam/skiaex.git",
    "deps_file"   : "DEPS",
    "managed"     : True,
    "custom_deps" : {
    },
    "safesync_url": "",
  },
  { "name"        : "src/third_party/skia",
    "url"         : "https://skia.googlesource.com/skia.git",
    "deps_file"   : "DEPS",
    "managed"     : False,
    "custom_deps" : {
    },
    "safesync_url": "",
  },

]
cache_dir = None
```
- Get all the sources

```gclient sync``` should bring this app together with skia into your working folder.

- Generate build files and run ninja build of the Skia
```
$ cd src
$ bin/fetch-gn
$ cd third_party/skia
$ gn gen out/Debug
$ ninja -C out/Debug
$ cd -
```

- Generate build files and run ninja build of skiaex
```
$ gn gen out/Debug
$ ninja -C out/Debug using_skia
```

- Run built binary on Linux
```
LD_LIBRARY_PATH=/usr/local/lib out/Debug/using_skia -z 8 -f fonts/DejaVuSans.ttf -m 20 -w 600 -h 800 < app/main.cpp && xdg-open out-skiahf.pdf
```
