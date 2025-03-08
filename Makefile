LIBS = -lGL -ldl -lglfw

main: main.cpp
	g++ main.cpp src/glad.c setup.cpp -o gravity_sim -Iinclude $(LIBS)
