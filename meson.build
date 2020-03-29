project(
  'h2non-semver-c', 'c',
  version: '1.0.0',
  license: 'MIT',
  default_options: [
    'c_std=c99',
  ],
)

h2nonsemverc_src = [
  'semver.c',
]

h2nonsemverc_inc = include_directories('.')

h2nonsemverc_lib = library(
  ''.join(meson.project_name().split('-')), h2nonsemverc_src,
  version: meson.project_version(),
)

h2nonsemverc_dep = declare_dependency(
  link_with: h2nonsemverc_lib,
  include_directories: h2nonsemverc_inc,
)
