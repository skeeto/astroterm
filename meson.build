project('starsaver', 'c',
    default_options : ['warning_level=3', 'c_std=c99', 'buildtype=debugoptimized'],
)

project_source_files = []
project_header_files = []
project_dependencies = []
target_link_args = []
project_include_dirs = [
    include_directories('include'),
    include_directories('data'),
]
conf = configuration_data()

subdir('include')
subdir('src')
subdir('data')

cc = meson.get_compiler('c')

curses = dependency('curses', required : true)
math = cc.find_library('m', required : true)

project_dependencies += [curses, math]

# Define some curses preprocessor symbols
if curses.found()
    check_headers = [
        ['ncursesw/menu.h',     'HAVE_NCURSESW_MENU_H'],
        ['ncurses/menu.h',      'HAVE_NCURSES_MENU_H'],
        ['menu.h',              'HAVE_MENU_H'],
        ['ncursesw/curses.h',   'HAVE_NCURSESW_CURSES_H'],
        ['ncursesw.h',          'HAVE_NCURSESW_H'],
        ['ncurses/curses.h',    'HAVE_NCURSES_CURSES_H'],
        ['ncurses.h',           'HAVE_NCURSES_H'],
        ['curses.h',            'HAVE_CURSES_H'],
    ]
    foreach h : check_headers
        if cc.has_header(h.get(0))
            conf.set(h.get(1), 1)
        endif
    endforeach
endif

# Check what curses headers are available
if (conf.get('HAVE_NCURSESW_H', 0) == 1)
    # Have ncurses with wide char support
    target_link_args += '-lncursesw'
else
    if (conf.get('HAVE_NCURSES_H', 0) == 1)
        # Have ncurses without wide char support*
        # *on certain systems like macOS, the ncurses header includes wide char
        # functions
        warning('ncursesw header not found, which is needed for unicode ' + 
                'support. If you are on macOS, this can be ignored')
        target_link_args += '-lncurses'
    else
        if (conf.get('HAVE_CURSES_H', 0) == 1)
            # Have curses but not ncurses 
            warning('ncurses header not found. Ncurses is required for ' +
                    'certain features such as color support')
        endif
    endif
endif

executable(
    meson.project_name(),
    project_source_files,
    dependencies        : project_dependencies,
    include_directories : project_include_dirs,
    c_args              : ['-Wall', '-Wextra', '-Wno-unused-variable', '-Wno-unused-parameter', '-g'],
    link_args           : target_link_args,
    install             : true,
)