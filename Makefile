CXX = g++
CXXFLAGS = -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations -Wmissing-include-dirs -Wswitch-enum -Wswitch-default -Weffc++ -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers -Wnon-virtual-dtor -Woverloaded-virtual -Wpointer-arith -Wsign-promo -Wstack-usage=8192 -Wstrict-aliasing -Wstrict-null-sentinel -Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE -Iinclude

all: assembler spu

assembler: obj/assembly.o
	$(CXX) $(CXXFLAGS) $^ -o assembler

spu: obj/spu.o obj/stack.o obj/calc.o
	$(CXX) $(CXXFLAGS) $^ -o spu

obj/%.o: src/%.cpp
	if not exist obj mkdir obj
	$(CXX) $(CXXFLAGS) -c $< -o $@


clean:
	del /q /f assembler.exe spu.exe 2>nul || title clean
	rmdir /s /q obj assembler spu 2>nul || title clean

