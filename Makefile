#For Mac OSX
#CXX = clang++
#CXXFLAGS = -pedantic

#For linux
CXX = g++
CXXFLAGS = -pedantic

ROOTFLAGS = $(shell root-config --cflags --libs) -lSpectrum

TARGET = Scan.cpp

OBJS = Scan.o

OBJ = odesa_scan

$(OBJS): $(TARGET)
	$(CXX) -o $(OBJ) $(TARGET) $(CXXFLAGS) $(ROOTFLAGS) 

clean:
	-rm -f $(OBJ)
