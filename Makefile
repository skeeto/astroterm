CC      = cc
CFLAGS  = -Wall -Wextra \
  -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function \
  -g3 -fsanitize=undefined -fsanitize-undefined-trap-on-error
LDFLAGS =
INC     = -I. -Iinclude -Itest/third_party/unity-v2.6.0
LIBS   != pkg-config --cflags --libs ncursesw

sources != find src data -name '*.[ch]'
generated = \
  include/cities.h \
  include/bsc5_constellations.h \
  include/bsc5_names.h \
  include/bsc5.h

astroterm$(EXE): astroterm.c $(sources) $(generated)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INC) -o $@ astroterm.c $(LIBS) -lm
include/cities.h: data/cities.csv
	xxd -i $^ | sed -r 's/data_|_csv//g' >$@
include/bsc5_constellations.h: data/bsc5_constellations.txt
	xxd -i $^ | sed -r 's/data_|_txt//g' >$@
include/bsc5_names.h: data/bsc5_names.txt
	xxd -i $^ | sed -r 's/data_|_txt//g' >$@
data/bsc5:
	curl -L -o $@ http://tdc-www.harvard.edu/catalogs/BSC5
include/bsc5.h: data/bsc5
	xxd -i data/bsc5 | sed -r 's/data_//g' >$@

tests = \
  test/astro_test \
  test/bit_test \
  test/city_test \
  test/coord_test \
  test/core_test \
  test/drawing_test \
  test/stopwatch_test

test/astro_test: test/astro_test.c src/astro.c
	$(CC) $(CFLAGS) $(INC) -o $@ $< -lm
test/bit_test: test/bit_test.c src/bit.c
	$(CC) $(CFLAGS) $(INC) -o $@ $< -lm
test/city_test: test/city_test.c src/city.c $(generated)
	$(CC) $(CFLAGS) $(INC) -o $@ $< -lm
test/coord_test: test/coord_test.c src/coord.c
	$(CC) $(CFLAGS) $(INC) -o $@ $< -lm
test/core_test: test/core_test.c src/astro.c src/bit.c src/coord.c \
  src/core.c src/core_position.c src/parse_BSC5.c $(generated)
	$(CC) $(CFLAGS) $(INC) -o $@ $< -lm
test/drawing_test: test/drawing_test.c src/bit.c src/drawing.c
	$(CC) $(CFLAGS) $(INC) -o $@ $< $(LIBS) -lm
test/stopwatch_test: test/stopwatch_test.c
	$(CC) $(CFLAGS) $(INC) -o $@ $< $(LIBS) -lm

check: $(tests)
	for test in $(tests); do $$test; done
	rm -f fake_terminal.txt

clean:
	rm -f astroterm$(EXE) $(generated) $(tests) fake_terminal.txt
