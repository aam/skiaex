vars = {
  "root_dir": "src",
}

deps = {
  "src/tools/gyp": "https://chromium.googlesource.com/external/gyp.git",
  "src/third_party/harfbuzz": "https://github.com/behdad/harfbuzz.git",
  "src/third_party/freetype" : "http://git.savannah.gnu.org/cgit/freetype/freetype2.git",
  "src/third_party/cairo" : "git://anongit.freedesktop.org/git/cairo",
  "src/third_party/pixman" : "git://anongit.freedesktop.org/git/pixman.git",
}

hooks = [
  {
    # A change to a .gyp, .gypi or to GYP itself should run the generator.
    "name": "gyp",
    "pattern": ".",
    "action": ["python", "src/build/gyp_using_skia"]
  }
]
