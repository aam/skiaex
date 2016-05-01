{
  'targets': [
    {
      'configurations': {
        'Debug': { },
        'Release': { }
      },
      'target_name': 'using_skia',
      'type': 'executable',
      'dependencies': [
        'third_party/skia/gyp/skia_lib.gyp:skia_lib',
        'third_party/skia/gyp/pdf.gyp:pdf',
      ],
      'include_dirs': [
        'third_party/skia/include/config',
        'third_party/skia/include/core',
        '/usr/local/include/harfbuzz',
      ],
      'sources': [
        'app/main.cpp'
      ],
      'libraries': [
         '/usr/local/lib/libharfbuzz.so',
      ],
      'ldflags': [
        '-std=c++11',
      ],
      'cflags': [
        '-Werror', '-W', '-Wall', '-Wextra', '-Wno-unused-parameter', '-g', '-O0'
      ]
    }
  ]
}
