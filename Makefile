#For Mac OSX
#CXX = clang++
#CXXFLAGS = -pedantic

#For linux
CXX = g++
CXXFLAGS = -pedantic -g

ROOTFLAGS = $(shell root-config --cflags --libs) -lSpectrum

TARGET = ODeSA_Scan.cpp

OBJS = ODeSA_Scan.o

OUTPUT = odesa_wavedump_scan


$(OBJS): $(TARGET)
	$(CXX) -o $(OUTPUT) $(TARGET) $(CXXFLAGS) $(ROOTFLAGS) 

clean:
	-rm -f $(OUTPUT)
