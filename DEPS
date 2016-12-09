vars = {
  "root_dir": "src",
}

deps = {
  "buildtools":  "https://chromium.googlesource.com/chromium/buildtools.git@55ad626b08ef971fd82a62b7abb325359542952b",
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
