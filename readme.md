# vczp-devel Project

This project is a C application that uses the `gcc` compiler to build an executable. The included `Makefile` simplifies the process of building, installing, and uninstalling the project.

## Project Structure

- `src/` - Directory containing `.c` source files.
- `includes/` - Directory containing `.h` header files.
- `build/` - Directory where object `.o` files are generated.
- `dist/` - Directory where the final executable is placed.

## Requirements

- `gcc` - C Compiler
- Libraries: `fileinfo`, `z`, `cjson`, `vczp`
- Library include directories must be correctly configured on the system.

## Building the Project

To compile the project and generate the executable, run:

````bash
make
````

This will create the vczp-devel executable in the dist/ directory.

# Installing the Executable

To install the executable into the /usr/bin directory, use:

````bash
sudo make install
````

# Uninstalling the Executable

To remove the installed executable, use:

````bash
sudo make uninstall
````

# Cleaning Up Generated Files

To remove the generated files and directories, run:

````bash
make clean
````

This will delete the build/ and dist/ directories.

# Makefile Details

* CC: Defines the C compiler to use (in this case, gcc).
* CFLAGS: Compilation flags, including warnings and include directories.
* LDFLAGS: Linking flags, including libraries and library directories.
* TARGET: The name of the final executable.
* BUILD_DIR: Directory where object files are stored.
* SRC_DIR: Directory where source files are located.
* INCLUDE_DIR: Directory where header files are located.
* SRCS: List of source files.
* OBJS: List of corresponding object files.


## The main Makefile rules are:

* all: Builds the project.
* $(BUILD_DIR): Creates the build directory if it does not exist.
* $(TARGET): Compiles the executable from object files.
* $(BUILD_DIR)/%.o: Compiles .c files into .o object files.
* clean: Removes generated files.
* install: Copies the executable to /usr/bin.
* uninstall: Removes the executable from /usr/bin.

# Usage

````bash
vczp-devel [options] [arguments]
````