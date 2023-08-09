# ✨cstar

View stars in your terminal!

![](/assets/screenshot.png)

## Command Line Options

| Option                            | Description                                                               | Default               |
|-----------------------------------|---------------------------------------------------------------------------|-----------------------|
| **`--latitude, -a lat`**          | Latitude of observation in radians (measured East of the Prime Meridian)  | Boston, MA            |
| **`--longitude, -o long`**        | Longitude of observation in radians (measured North of the Equator)       | Boston, MA            |
| **`--julian-date, -j jd`**        | Julian date of observation                                                | Current system time   |
| **`--threshold, -t thresh`**      | Only stars brighter than this will be rendered                            | 3.0                   |
| **`--label-thresh, -l thresh`**   | Only stars brighter than this will be labeled                             | 0.5                   |
| **`--fps, -f fps`**               | Frames per second                                                         | 24                    |
| **`--animation-mult, -m mult`**   | Real time animation speed multiplier                                      | 1.0                   |
| **`--no-unicode`**                | Only use ASCII characters                                                 |                       |
| **`--colors`**                    | Render planets with terminal colors                                       |                       |
| **`--grid`**                      | Draw an azimuthal grid                                                    |                       |
| **`--constellations`**            | Draw constellations                                                       |                       |

> ℹ️ Use a tool like https://www.aavso.org/jd-calculator to convert Gregorian Calendar  Dates to Julian Dates \
> Use a tool like https://www.latlong.net/ to get your latitude and longitude (remember to convert to radians!)

> ℹ️ Star magnitudes decrease as apparent brightness increases

## Building

Compile with gcc:
```
gcc main.c bit.c term.c coord.c astro.c parse_BSC5.c cstar.c -lm -lncursesw
```
_You only need to link `-lncurses` if not using unicode_

## Data Sources
- Stars: [Yale Bright Star Catalog](http://tdc-www.harvard.edu/catalogs/bsc5.html)
- Star Names: [IAU Star Names](https://www.iau.org/public/themes/naming_stars/)
- Constellation Figures: [Stellarium](https://stellarium.org/) *modified
- Planets: [NASA Jet Propulsion Laboratory](https://ssd.jpl.nasa.gov/planets/approx_pos.html)