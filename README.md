# root_to_json

`to_json` is a simple application to export primitive hierarchy of ROOT objects
(TVector, THist, ..) objects to a json format. Currently it can export the
following objects:
  - TH1D, TH2D
  - TGraph
  - TVector
  - CAFAna/Spectrum
  - CAFAna/Surface


## Usage

Usage of `root_to_json` is simple:

```
$ root_to_json INPUT.root OUTPUT.json
```


## Build Instructions

To compile `root_to_json` program you would need to have ROOT and BOOST
available. The compilation process can be started by running

```
$ make
```

Feel free to tweak `Makefile` if the build process fails to find some of the
ROOT/BOOST libraries.

