CXXFLAGS=-std=c++17 -Iezpwd-reed-solomon/c++ -ggdb3
LDFLAGS=-ggdb3
OBJS=pom.o

pom : $(OBJS)
	$(CXX) -o pom $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) pom
