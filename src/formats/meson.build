FORMATS = [
  '40track_drive',
  'acornadfs',
  'acorndfs',
  'aeslanier',
  'agat840',
  'amiga',
  'ampro',
  'apple2_drive',
  'apple2',
  'appledos',
  'atarist360',
  'atarist370',
  'atarist400',
  'atarist410',
  'atarist720',
  'atarist740',
  'atarist800',
  'atarist820',
  'bk800',
  'brother120',
  'brother240',
  'commodore1541',
  'commodore1581',
  'eco1',
  'f85',
  'fb100',
  'hp9121',
  'hplif770',
  'ibm1200',
  'ibm1232',
  'ibm1440',
  'ibm180',
  'ibm360',
  'ibm720',
  'ibm',
  'mac400',
  'mac800',
  'micropolis143',
  'micropolis287',
  'micropolis315',
  'micropolis630',
  'mx',
  'n88basic',
  'northstar175',
  'northstar350',
  'northstar87',
  'prodos',
  'rx50',
  'shugart_drive',
  'tids990',
  'vgi',
  'victor9k_ds',
  'victor9k_ss',
  'zilogmcz',
]

protoencode = executable(
  'protoencode',
  '../../scripts/protoencode.cc',
  dependencies: [
    config_proto_dep,
    fmt_dep,
    protobuf_dep,
    pthreads_dep,
  ],
  native: true
)

mktable = find_program('../../scripts/mktable.sh')

generated_files = []
foreach format : FORMATS
  generated_files += custom_target(
    format + '_format',
    output: [ format + '_format.cc' ],
    input: [ format + '.textpb' ],
    command: [ protoencode, '@INPUT0@', '@OUTPUT@', 'formats_' + format + '_pb' ],
  )
endforeach

generated_files += custom_target(
  'format_table',
  output: [ 'format_table.cc' ],
  command: [ mktable, 'formats', FORMATS ],
  capture: true
)

formats_dep = declare_dependency(
    link_with: [
      static_library(
        'formats',
        sources: [
          generated_files,
        ],
      )
    ]
)
