CXXFLAGS += `pkg-config --cflags opencv4`
LDFLAGS += `pkg-config --libs opencv4`

all : color_correction

clean : rm -f color_correction

color_correction : color_correction.cc
	$(CXX) $(CXXFLAGS) -Wall -o $@ $< $(LDFLAGS)
