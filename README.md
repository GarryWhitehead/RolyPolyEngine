# RolyPolyEngine (RPE) #

Yes, another Vulkan enigne. 

What's the purpose of the RolyPoly Engine? 
Mainly an experiment on my part to try and write an engine in C only, with no use of the overly-bloated C++ or the std library headers. 

**Note:** This is very much a work in progress. There are bugs and imperfections, these will be ironed out over time with the addition of more functionality (see the list of work to do below). However, the API is in working order and I'm hoping that breaking changes can be avoided.

## Building ##

YAVE utilises the package manager [Conan](https://docs.conan.io/2/) to try and make the configuration and build process as pain-free as possible.  

To begin with, you will need to download Conan using pip if not already present on your system:

```
$ pip install conan --user
```

**Note:** Python3.x is required to use Conan. If pip isn't installed on your system, information on installing can be found [here](https://pip.pypa.io/en/stable/installation/)    
**Note:** A minimum version of Conan 1.53.0 is required due to recipe requirements in the Conan Center.    

To setup your system for use with the YAVE library run:

```
$ ./install.sh
```

Now build the third-party libraries required by YAVE using Conan:

```
$ mkdir build && cd build
$ conan install .. -pr:b=default -pr:h=default --build=missing -s build_type=Release
```

Now run cmake to create the configuration files:

```
$ cmake .. -DCMAKE_TOOLCHAIN_FILE="Conan/conan_toolchain.cmake" -G "Unix Makefiles"
```

**Note:** You can replace `Unix Makefiles` with whichever generator you wish to use.    

Then assuming you generated makefiles, you can build all libraries/exectuables using:

```
$ cmake --build . --config Release
```


## Features to add / Issues to address ##

In some order of priority... maybe.

- [ ] IBL
- [ ] Optimisations (including use of Structure of Arrays)
- [ ] Add multi-threading (asset loading, pipeline generation, cmd buffers, etc.) 
- [ ] Shadow mapping (and cascades)
- [ ] Skinning / Animation
- [ ] Post-processing effects
- [ ] Assimp model loading
- [ ] Terrian generation
- [ ] Wave simulation

## Third-party Libraries ##

Many thanks to the authors of the following third-party libraries which are used by YAVE:

- AMD VulkanMemoryAllocator
- GLFW
- stb
- log.c
- cgltf
- jsmn
- libktx

## Assets ##

At present, the assets are cloned from the Khronos asset repository which can be found [here](https://github.com/KhronosGroup/Vulkan-Samples-Assets). This includes all licenses and are only used for testing the engine. In the future, I will add just a select few assets though would prefer to host them somewhere ouside of this repo to keep git clone times reasonable.
