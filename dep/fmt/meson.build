fmt_includes = include_directories('.')

fmt_dep = declare_dependency(
    include_directories: [
      fmt_includes,
    ],
    link_with: [
      static_library(
        'fmt',
        sources: [
          'format.cc',
          'os.cc',
        ],
        include_directories: [
          fmt_includes,
        ]
      )
    ]
)



