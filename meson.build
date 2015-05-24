project('xcape', 'c')
executable(
  'xcape',
  sources : 'xcape.c',
  dependencies : [dependency('x11'), dependency('xtst')],
  link_args : '-pthread',
  install : true
)
install_man('xcape.1')
