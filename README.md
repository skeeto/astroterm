# ✨starsaver

Stellar magic, now in your terminal! ✨🪐 See stars, planets, constellations, and more, all rendered right the command line—no telescope required 🌌

![Screenshot of Starsacer](/assets/screenshot.png)

## Building

### Requirements

- Linux or macOS
- [Meson](https://github.com/mesonbuild/meson) 0.49 or newer
- [Ninja](https://github.com/ninja-build/ninja) 1.8.2 or newer
- A C compiler
- ncurses library

### Installation

> Ncurses detection is spotty on some systems, and you may need to install [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) in order for Meson to find it.
>
> You may install via [Homebrew](https://formulae.brew.sh/formula/ncurses) on MacOS.

Clone this repo to your local system and enter the project directory.

```bash
git clone https://github.com/da-luce/starsaver && cd starsaver
```

Initialize the builder.

```bash
meson setup build
```

To rebuild from then on from within the newly created `../build/` directory.

```bash
cd build && meson compile
```

To install to default location (this may require sudo privileges):

```bash
meson install
```

_Note, after rebuilding any changes, rerunning `meson install` is required_

### [Known issues](https://github.com/da-luce/starsaver/issues)

### Testing

Run `meson test` within the build directory.

### Coverage

```bash
meson test
ninja coverage
```

## Usage

After installing _starsaver_, simply run `starsaver` to invoke it.

```bash
starsaver
```

Pressing ESC will exit.

### Options

Add the `--help` flag to view all supported options. Here's an example use case.
Say we wanted to view the sky at 5:00 AM on July 16, 1969—the morning of the
Apollo 11 launch at the Kennedy Space Center in Florida. We would run:

```bash
starsaver --latitude 28.573469 --longitude -80.651070 --datetime 1969-7-16T9:32:00
```

If we then wanted to display all stars with a magnitude brighter than or equal
to 5.0 and add color, we would add `--threshold 5.0 --color` as options.

If you simply want the current time, don't specify the `--datetime` option and
_starsaver_ will use the system time. For your current location, you will still
have to specify the `--lat` and `--long` options.

For more options and help run `starsaver -h` or `starsaver --help`

> ℹ️ Use a tool like https://www.latlong.net/ to get your latitude and longitude
> ℹ️ Star magnitudes decrease as apparent brightness increases

## Citations

> Many thanks to the following resources, which were invaluable to the development of this project.

- [Map Projections-A Working Manual By JOHN P. SNYDER](https://pubs.usgs.gov/pp/1395/report.pdf)
- [Wikipedia](https://en.wikipedia.org)
- [Atractor](https://www.atractor.pt/index-_en.html)
- [Jon Voisey's Blog: Following Kepler](https://jonvoisey.net/blog/)
- [Celestial Programming: Greg Miller's Astronomy Programming Page](https://astrogreg.com/convert_ra_dec_to_alt_az.html)

## Data Sources

- Stars: [Yale Bright Star Catalog](http://tdc-www.harvard.edu/catalogs/bsc5.html)
- Star names: [IAU Star Names](https://www.iau.org/public/themes/naming_stars/)
- Constellation figures: [Stellarium](https://stellarium.org/)
- Planet orbital elements: [NASA Jet Propulsion Laboratory](https://ssd.jpl.nasa.gov/planets/approx_pos.html)
- Planet magnitudes: [Computing Apparent Planetary Magnitudes for The Astronomical Almanac](https://arxiv.org/abs/1808.01973)