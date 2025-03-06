# 3D Visualization Tool

## Description
This project is a 3D visualization tool developed using OpenGL and FreeGLUT. It allows users to create and manipulate 3D graphics in real-time.

## Features
- Real-time rendering of 3D objects.
- Interactive controls for manipulating the view.
- Support for various 3D models and textures.

## Installation
To set up the project, ensure you have the following dependencies installed:
- OpenGL
- FreeGLUT

You can install these libraries using your package manager. For example, on Ubuntu, you can run:
```bash
sudo apt-get install freeglut3-dev
```

## How to Compile
To compile the project, use the following command:
```bash
g++ main.cpp -o main -lglut -lGLU -lGL -L/usr/local/lib -lSOIL    
```

## How to Run
After compiling, run the application with:
```bash
./main
```

## License
This project is licensed under the MIT License.
