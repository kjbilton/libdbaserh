# libdbaserh 0.3
`libdbaserh` is a C library for device control and data acquisition of DigiBase-RH PMT bases. The library is an adaption of `libdbase` 0.2 for DigiBase-RH modules.

The manufacturer's explanation of the difference between DigiBase and DigiBase-RH (April 2012):

>"RH version was designed to be more ruggedized against neutron damage which
> was a problem with its predecessor. When this change was made, much of the
> hardware and its method of communication changed as well."

## Development Status
- version 0.3 -- set of different zero and gain stabilization during
                 device initialization (file libdbaserhi.c),
                 modification of some of these parameters on the initialized
                 device is not working

- version 0.2 -- the detector status after init process changed
                 (real/live time preset to 0)

- version 0.1 -- initial version

##  TODO
- Write a comprehensive manual.
- Display actual HV
- Handle status messages (are there any?)

## Dependencies
- `libusb-1.0` for low-level USB communication

## Installation
1. Install `libusb-1.0`.
1. Locate `digiBaseRH.rbf` on the Windows system with the ORTEC DigiBase software installed (the default installation path seems to be C:\Windows\System32\digiBase.rbf).
    - Note: md5sum of digiBaseRH.rbf should be `8ef9cdf4eb72d234a8add7afee4963ba`
1. Copy this file to your Linux system and specify its path in the Makefile.
  (Note: Leave the capital 'B' in the filename, it should be 'digiBaseRH.rbf').
  This path will be hard coded into libdbase during compilation.
1. Edit the Makefile to suit your installation.
1. Compile the library: ```make```
1. Install the library: ```sudo make install```
1. Run the control program: ```sudo ./dbaserh```

## Acknowledgements
`libdbaserh` originally comes from [Jan Kamenik](jankamenik@gmail.com). It is an extension of `libdbase`, written by [Peder Kock](peder.kock@med.lu.se) and Jonas Nilsson,
Lund University. See http://libdbase.sourceforge.net for more information on `libdbase`.

## License
`libdbaserh` is licensed under the GPL-3.

### License Note
We cannot distribute the non-free firmware you need to use this library, but since you probably already have purchased a copy of Maestro, you are free to copy the firmware from one installation (Windows) to another (Linux), as long as you don't redistribute it.
