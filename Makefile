# Compiler stff
CPP?=clang++
CPPFLAGS?=-g -Wall -Werror -D CBF -std=c++11 -O2 

# Source File stuff
INCDIR=include
LIBDIR=lib
BINSRC=src
BINDIR=bin
OBJDIR=obj

CC_FILES=$(wildcard $(LIBDIR)/*.cc)
CPP_FILES=$(wildcard $(BINSRC)/*.cpp) $(wildcard $(BINSRC)/hoc/*.cpp) $(wildcard $(BINSRC)/fe_hoc/*.cpp)

# Includes
INC=-I./$(INCDIR)

# Out Objects
OBJECTS=$(addprefix $(OBJDIR)/,$(notdir $(CC_FILES:.cc=.o)))
TARGET_O_LIST=$(addprefix $(OBJDIR)/,$(notdir $(CPP_FILES:.cpp=.bin)))
TARGET_LIST=$(addprefix $(BINDIR)/,$(notdir $(CPP_FILES:.cpp=)))

.PHONY: directories

all: directories $(TARGET_LIST)

$(BINDIR)/%: $(OBJECTS) $(OBJDIR)/%.bin
	$(CPP) $(CPPFLAGS) -o $@ $^

$(OBJDIR)/%.bin: $(BINSRC)/%.cpp 
	$(CPP) $(CPPFLAGS) $(INC) -c $< -o $@

$(OBJDIR)/%.bin: $(BINSRC)/hoc/%.cpp 
	$(CPP) $(CPPFLAGS) $(INC) -c $< -o $@

$(OBJDIR)/%.bin: $(BINSRC)/fe_hoc/%.cpp 
	$(CPP) $(CPPFLAGS) $(INC) -c $< -o $@

$(OBJDIR)/%.o: $(LIBDIR)/%.cc
	$(CPP) $(CPPFLAGS) $(INC) -c $< -o $@

directories: ${OBJDIR} ${BINDIR}

${OBJDIR}:
	mkdir -p ${OBJDIR}

${BINDIR}:
	mkdir -p ${BINDIR}

.PHONY: clean
clean:
	@echo rm $(TARGET_LIST)
	@rm $(TARGET_LIST)
