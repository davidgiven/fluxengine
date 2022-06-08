stb_includes = include_directories('.')

stb_dep = declare_dependency(
    include_directories: [
      stb_includes,
    ],
    link_with: [
      static_library(
        'stb',
        sources: [
          'stb_image_write.c',
        ],
        include_directories: [
          stb_includes,
        ]
      )
    ]
)



