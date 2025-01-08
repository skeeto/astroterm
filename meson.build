project('astroterm', 'c',
    version: '1.0.1',
    default_options : ['warning_level=3', 'buildtype=minsize'],
    meson_version: '>=1.4.0',
)

# Detect runtime environment
env = environment()
is_release = get_option('release_build')
is_windows = host_machine.system() == 'windows'

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

conf = configuration_data()

# Build submodules
project_dependencies = []
project_source_files = []
project_header_files = []
project_include_dirs = [
    include_directories('include'),
    include_directories('data'),
]

subdir('include')
subdir('src')
subdir('data')

# Setup compiler specific flags, and optimize for size if running in CI
cc = meson.get_compiler('c')
if cc.get_id() == 'msvc'
    # MSVC
    cflags = ['/O1', '/W4', '/utf-8', ]
    if not is_release
        cflags += ['/Zi']
    endif
else
    # For GCC or other compilers
    cflags = ['-O2', '-Wall', '-Wno-unused-variable', '-Wno-unused-parameter', '-Wunused-function']
    if not is_release
        cflags += ['-Wextra', '-g']
        message('Adding profiling')
    else
        cflags += ['-s']
        message('Stripping debug symbols')
    endif
endif

if is_windows

    # This is where we will install Windows libraries
    local_include = include_directories('install/include')
    local_lib = join_paths(meson.current_source_dir(), 'install/lib')
    project_include_dirs += local_include

    # PDcurses
    curses = declare_dependency(
        include_directories: local_include,
        link_args: ['-L' + local_lib, '-lpdcurses']
    )

    if curses.found()
        message('PDCurses found!')
    else
        message('PDCurses not found. Please ensure it is installed and the paths are correct.')
    endif

    argtable2 = declare_dependency(
        include_directories: local_include,
        link_args: ['-L' + local_lib, '-largtable2']
    )

    if argtable2.found()
        message('argtable2 found!')
    else
        message('argtable2 not found. Please ensure it is installed and the paths are correct.')
    endif

else
    curses = dependency('curses', required : true, static: true)
    argtable2 = dependency('argtable2', required: true, static: true)
endif

# TODO: why is this necessary?
if is_windows
    # Skip the math library check on Windows
    # FIXME: this is messy
    math = dependency('dummy', required: false)
else
    math = cc.find_library('m', required : true)
endif

# ------------------------------------------------------------------------------
# Curses info
# ------------------------------------------------------------------------------

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
else
    if (conf.get('HAVE_NCURSES_H', 0) == 1)
        # Have ncurses without wide char support*
        # *on certain systems, the ncurses header includes wide char functions
        warning('ncursesw header not found, which is sometimes needed for ' +
                'unicode support. This may not be an issue as the regular ' +
                'ncurses.h sometimes contains wide character support')
    else
        if (conf.get('HAVE_CURSES_H', 0) == 1)
            # Have curses but not ncurses
            warning('ncurses header not found. Ncurses is required for ' +
                    'certain features such as color support')
        endif
    endif
endif

# ------------------------------------------------------------------------------
# Embed data
# ------------------------------------------------------------------------------

if is_windows
    # On Windows, use python script
    embed_command = [
        'cmd', '/c', 'python', '..\scripts\embed.py', '@INPUT@', '@OUTPUT@'
    ]
else
    # On Unix-like systems, use the shell script
    embed_command = [
        'sh', '../scripts/embed.sh', '@INPUT@', '@OUTPUT@'
    ]
endif

bsc5_data = configure_file(
    input: 'data/bsc5',
    output: 'bsc5_data.h',
    command: embed_command
)
bsc5_constellations = configure_file(
    input: 'data/bsc5_constellations.txt',
    output: 'bsc5_constellations.h',
    command: embed_command
)
bsc5_names = configure_file(
    input: 'data/bsc5_names.txt',
    output: 'bsc5_names.h',
    command: embed_command
)
city_data = configure_file(
    input: 'data/cities.csv',
    output: 'city_data.h',
    command: embed_command
)
project_source_files += [bsc5_data, bsc5_constellations, bsc5_names, city_data]

# ------------------------------------------------------------------------------
# Application library (for reusability)
# ------------------------------------------------------------------------------

project_lib = static_library(
    'astroterm_lib',
    project_source_files,
    dependencies        : [curses, math],
    include_directories : project_include_dirs,
    c_args              : cflags
)

# ------------------------------------------------------------------------------
# Application executable
# ------------------------------------------------------------------------------

# Get the version from the project
program_version = meson.project_version()
program_name = meson.project_name()

# Add preprocessor definitions
build_defines = [
  f'-DPROJ_NAME="@program_name@"',
  f'-DPROJ_VERSION="@program_version@"'
]

executable(
    meson.project_name(),
    'src/main.c',
    link_with           : project_lib,
    dependencies        : [argtable2],
    include_directories : project_include_dirs,
    c_args              : cflags + build_defines,
    install             : true,
)

# ------------------------------------------------------------------------------
# Testing
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Unity Static Library
# ------------------------------------------------------------------------------

test_files = []
test_infra_source_files = []
test_include_dirs = []
subdir('test')

unity_lib = static_library(
    'unity',
    test_infra_source_files,
    include_directories: test_include_dirs,
    c_args: cflags
)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

# Compile test modules separately
# DUNITY_INCLUDE_CONFIG_H is necessary for testing doubles
foreach test_file : test_files
    filepath = test_file[0].full_path()
    if is_windows
        # On Windows, use backslashes
        filename = filepath.split('\\')[-1]
    else
        # On non-Windows, use forward slashes
        filename = filepath.split('/')[-1]
    endif

    # Call the custom function to get the filename without the extension
    test_name = filename.split('.')[0]
    test_exe = executable(
        test_name,
        [test_file] + test_infra_source_files,
        link_with: [project_lib, unity_lib],
        include_directories: project_include_dirs + test_include_dirs,
        c_args: cflags + '-DUNITY_INCLUDE_CONFIG_H',
        install: false
    )

    test(test_name, test_exe)
endforeach
