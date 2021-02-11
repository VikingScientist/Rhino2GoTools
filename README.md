# Rhino2GoTools

This is a CLI tool for converting native files from the commercial CAD system [Rhinosceros 3D](https://www.rhino3d.com/) (.3dm) to [GoTools](https://www.sintef.no/projectweb/geometry-toolkits/gotools/) file format (.g2).

Note that this only supports "nice" spline geometries. The script only picks up any SplineCurve and untrimmed SplineSurface objects. If the objects cannot be converted into either of these two without loss of accuracy or human-in-the-loop, then it is unsopprted and simply skipped.


## Dependencies

This depends on two libraries in particular: GoTools and opennurbs. The easiest way to get these is to install precompiled versions from the [IFEM team](https://launchpad.net/~ifem/+archive/ubuntu/ppa/+index?field.series_filter=xenial) at launchpad.

```
sudo add-apt-repository ppa:ifem/ppa
sudo apt-get update
sudo apt install libgotools-core-dev
sudo apt install libopennurbs1-dev
```

## Compilation
This should compile by simply running `make` in the project root folder. The software has been developed and tested on Linux systems which (Ubuntu) and compilation will probably fail on other systems.

## Use
After compilation you will have the executable `3dm_2_g2` file in your folder which takes as input argument a `.3dm` file and produces a `.g2` file as output. Test this with the provided example files in `TestFiles` folder.
```
./3dm_2_g2 TestFiles/2D.3dm
```
