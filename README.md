> This offering is not approved or endorsed by OpenCFD Limited, producer
> and distributor of the OpenFOAM software via 
> [www.openfoam.com](https://www.openfoam.com), and owner of the 
> OPENFOAM® and OpenCFD® trade marks

> This project uses core libraries from
> [rheoTool](https://github.com/fppimenta/rheoTool)
> to implement the interface between OpenFOAM and PetSc

# Open Reservoir Simulation Research tool

This toolbox makes use of (nearly) latest OpenFOAM and PetSc code to build capable
(but not yet efficient enough) solvers for BlackOil equations in isotherm porous media.
It's thanks to the OpenFOAM developpers that this tool exists. 
I hope this project could contribute to theirs at some point.

> This is version 0.1, so you can expect things to break easily.
> - It's going through a rewrite here at OpenRSR-OF7 repo, when it's finished
  we'll move back to OpenRSR
> - I'll try to keep the master branch clean and functional
> - Parallel simulations will (virtually) work as long as you don't use `labelToCell` to
>   specify well cells, or split a single well's cells to many processor. Otherwise, it's
>   a bug which you should report.

## Introduction

> There is no three-phase models (yet) but the infrastructure is correctly
> templated and you could easily build some.

I developed this library originally for my Master Thesis project to show the potential of 
some Open Source software (OpenFOAM) in Petroleum Reservoir Simulation. So,
the code was fairly simple, with no extensive use of templates/advanced C++
stuff. But this is changing as I long for more performance and generic
models.

The whole purpose of the library is to provide open-access for reservoir engineers to go 
beyond classic simulations and research their own ideas easily, with no third party 
(Giant Oil & Gas Companies) telling them what models to use and what's not. People should 
always have access to the source code of the software they use, especially when scientific
tasks are involved.

Coupled solvers are probably the best choice in many cases, so stick with them until a
fully implicit solver (Newton-Raphson) solver is developed; or develop your own. :point_left:

## Installation and Usage Instructions

### Compiling the library

To compile all library parts to binary shared objects and
executables
(`Allwmake` script should have execution permissions,
it will also compile all solvers):

```sh
./Allwmake
```

Take a look at `Make/options` for each library department to change 
compiled classes and library name, ..., etc. I assume you're familiar with this kind of
situations.

### Use in own solvers

One can dynamically link some libraries in `controlDict` of a simulation 
case by adding the following entry:

```cpp
libs
(
    libmyOwnKrModels.so
);
```

Which should allow for use of custom relative permeability models (for example) 
without modifying existing solvers (Like UDFs in commercial software).

If a custom solver is to instantiate an object with the help
of some library class, the library must be statically linked (At compile time).
Here is an example, assuming `relativePermeabilityModel` is an abstract type
which has a `::New` method returning a pointer to the chosen concrete type:

```cpp
// Include parent virtual class header file for all Kr models
#include "relativePermeabilityModel.H"

// Now we can construct the model using (static) New method
// And read its parameters from constant/transportProperties
autoPtr<relativePermeabilityModel> krModel = 
        relativePermeabilityModel::New("krModel", transportProperties, ...);

krModel->correct(); // Most models have a correct method which 
                    // calculates or updates model results/params.

// Which krModel did we correct?
// Well, it was chosen in transportProperties file
```

And make sure the solver's `Make/options` statically links the library in
question:

```sh
EXE_INC = \
    # Some lnIncludes the solver already has
    -I../../libs/permeabilityModels/lnInclude # <-- path to library sources

EXE_LIBS = \
    # Some libraries the solver already loads
    -L$(FOAM_USER_LIBBIN) \ # <-- if the library is located here (it should)
    -lrelativePermeabilityModels  # <-- load the actual library
    # This assumes the shared object is called librelativePermeabilityModels.so
```

> Changing library code will take effect immediately unless you make changes
> to the virtual parent class itself; There is no need to re-compile solvers.

### Compiling native Windows executable

Never tried it, and probably wouldn't try it in the near futur. But I would be
happy to see someone have a go at it (OpenFOAM is not that hard to port).

# Try the toolbox right NOW

> To try the toolbox's solvers, you don't have to install anything on your
> machine

This section describes how to quickly run the Buckley-Levrett case in a Docker
container:

1. If you have Docker installed, please use it; else, head over to
   [The Playground](https://labs.play-with-docker.com/) and start a instance
   there.
2. `docker run -it -u root openfoam/openfoam7-paraview56` to create an OpenFOAM
   container and get a root shell in there.
3. Install needed software with:
   `apt update && apt install git wget curl vim nano tar -y`
4. Switch to a regular user `su - openfoam`
5. Clone development branch `git clone --depth 1 --branch=develop https://github.com/FoamScience/OpenRSR-OF7`
6. Compile a recent Petsc with `cd OpenRSR-OF7; ./scripts/installPetsc.sh`
   (The one from Ubuntu repos is not compatible)
6. Compile the toolkit (will take 3-5mins) `./Allwmake`
7. Run the case `cd tutorials/blackOil/BuckleyLeverett; ./AllRun`.
   ( Check the case's
   [README](https://github.com/FoamScience/OpenRSR-OF7/tree/develop/tutorials/blackOil/BuckleyLeverett)
   )
8. Compress the whole case directory `tar -zcvf BL.tar.gz .`
9. Upload the compressed file to `file.io` for example
   `curl -F "file=@BL.tar.gz" https://file.io/?expires=1d`
10. Now download the case to your machine and take a look at results folder
    (Or load the case into ParaView and investigate it)
