CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra

all: parent child

parent: parent.cpp
	$(CXX) $(CXXFLAGS) -o parent parent.cpp

child: child.cpp
	$(CXX) $(CXXFLAGS) -o child child.cpp

clean:
	rm -f parent child
