{
  'conditions': [
    ['OS=="linux"', {
      'variables' : {
        # Find the pull path to the pg_config command, since iy may not be on the PATH
        'pgconfig': '<!(which pg_config || find /usr/bin /usr/local/bin /usr/pg* /opt -executable -name pg_config -print -quit)'
      }
    }, {
      #Default to assuming pg_config is on the PATH.
      'variables': {
        'pgconfig': 'pg_config'
      },
    }]
  ],
  'targets': [
    {
      'target_name': 'addon',
      'sources': [
        'src/connection.cc',
        'src/connect-async-worker.cc',
        'src/addon.cc'
      ],
      'include_dirs': [
        '<!@(<(pgconfig) --includedir)',
        '<!(node -p "require(\'node-addon-api\').include_dir")'
      ],
      'defines': ['NAPI_CPP_EXCEPTIONS'],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'conditions' : [
        ['OS=="linux"', {
            'cflags': [
                  '-fvisibility=hidden',
                  # '-fsanitize=address',
                  # '-fsanitize-recover=address',
                  # '-fno-omit-frame-pointer',
                  # '-O1'
            ]
        }],
        ['OS=="win"', {
          'libraries' : ['ws2_32.lib','secur32.lib','crypt32.lib','wsock32.lib','msvcrt.lib','libpq.lib'],
          'defines': [
            '_HAS_EXCEPTIONS=1'
          ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': 1
            },
            'VCLinkerTool' : {
              'AdditionalLibraryDirectories' : [
                '<!@(<(pgconfig) --libdir)\\'
              ]
            },
          }
        }, { # OS!="win"
          'libraries' : ['-lpq -L<!@(<(pgconfig) --libdir)'],
          'ldflags' : [ '<!@(<(pgconfig) --ldflags)']
        }],
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'CLANG_CXX_LIBRARY': 'libc++',
            'MACOSX_DEPLOYMENT_TARGET': '10.7',
            'OTHER_CFLAGS': [
                  '-fsanitize=address',
                  '-fsanitize-recover=address',
                  '-fno-omit-frame-pointer',
                  '-O1'
            ],
            'OTHER_LDFLAGS': [
                '-fsanitize=address'
            ]
          }
        }]
      ]
    }
  ]
}
