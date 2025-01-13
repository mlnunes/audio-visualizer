# audio-visualizer
Fork from original https://terminalroot.com.br/2024/12/como-criar-um-visualizador-de-Audio-com-cpp.html

Audio Spectrum with SFML and FFTW


---

## Dependencies
+ C++ compiler: [GNU GCC](https://gcc.gnu.org/)
+ [SFML](https://www.sfml-dev.org/)
+ [FFTW](https://fftw.org/)

Example of installing FFTW on Fedora:
```bash
run0 dnf install fftw-devel
```

---

## Compile and run
```bash
make
./audio_pl Music.mp3
```

---
## Changes from original:
- volume control
- audio progress bar
- comments included in the source code
- make file
