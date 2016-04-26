vars = {
  "root_dir": "src",
}

deps = {
  "src/tools/gyp": "https://chromium.googlesource.com/external/gyp.git",
}

hooks = [
  {
    # A change to a .gyp, .gypi or to GYP itself should run the generator.
    "name": "gyp",
    "pattern": ".",
    "action": ["python", "src/build/gyp_using_skia"]
  }
]
