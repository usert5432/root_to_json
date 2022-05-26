TARGET = root_to_json
SRC    = root_to_json.C

INC = -I${BOOST_INC}
LIB = $(shell root-config --libs) -lstdc++

CPPFLAGS ?= -O3
CXXFLAGS += $(shell root-config --cflags) $(CPPFLAGS) $(INC) --std=c++17 -Wall
LDFLAGS  += $(shell root-config --ldflags) $(LIB)

.PHONY : clean

$(TARGET) : $(SRC)
	$(CC) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

