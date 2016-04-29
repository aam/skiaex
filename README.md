# skiaex

This sample demonstrates how to use Skia and HarfBuzz with Freetype to produce pdf file from a given stdin.

HarfBuzz and its prerequisites
===

You have to install HarfBuzz and Freetype2 if you don't have them on your machine.

For example, like this:

```
sudo apt-get install autoconf automake libtool pkg-config ragel gtk-doc-tools
git clone https://github.com/behdad/harfbuzz.git
cd harfbuzz
./autogen.sh
./configure
make && sudo make install
```

```
git clone http://git.savannah.gnu.org/cgit/freetype/freetype2.git
cd freetype2
./autogen.sh
./configure
make && sudo make install
```

Build and run with Skia cmake build
===
- Get Skia sources and run cmake build of Skia
```
cd $HOME
git clone https://skia.googlesource.com/skia.git
cd skia/cmake
cmake . -G Ninja
ninja skia
```
- Get the source code for this app
```
cd $HOME
git clone https://github.com/aam/skiaex.git
```
- Build it
```
cd $HOME/skiaex  
c++ @${HOME}/skia/skia/cmake/skia_compile_arguments.txt -I/usr/local/include/harfbuzz -I/usr/local/include/freetype2  app/main.cpp @${HOME}/skia/skia/cmake/skia_link_arguments.txt -L/usr/local/lib -lharfbuzz -lfreetype -o using_skia
```
- Run it to produce pdf file with it's own source code
```
./using_skia -z 8 -f fonts/DejaVuSans.ttf -m 20 -w 600 -h 800 < app/main.cpp && xdg-open out-skiahf.pdf
```

Build and run with gyp
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

- Run ninja build
```
cd src
ninja -C out/Debug using_skia
```

- Run it to produce pdf file with it's own source code
```
out/Debug/using_skia -z 8 -f fonts/DejaVuSans.ttf -m 20 -w 600 -h 800 < app/main.cpp && xdg-open out-skiahf.pdf
````
