project('starsaver', 'c',
    default_options : ['warning_level=3', 'c_std=c99', 'buildtype=debugoptimized'],
)

project_source_files = []
project_header_files = []

project_dependencies = [
    # TODO: find how to recuire ncursesw. Does this cover pdcurses?
    dependency('curses', required : true),
    meson.get_compiler('c').find_library('m', required : true),
]

project_include_dirs = [
    include_directories('include'),
    include_directories('data'),
]

subdir('include')
subdir('src')
subdir('data')

build_args_c = ['-Wall','-Wextra','-g']

executable(
    meson.project_name(),
    project_source_files,
    dependencies        : project_dependencies,
    include_directories : project_include_dirs,
    install             : true,
    c_args              : build_args_c,
)