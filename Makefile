# Player de audio com visualizador de espectro de audio
# fonte: Terminalroot
#

CXX = /usr/bin/g++
MAINSRC = main.cpp
MAIN = audiopl
NFD_path = nfd/
NFD_Include = include/
NFD_Load = x64/
NFD_cxx_flags = -I$(NFD_path)$(NFD_Include) -L$(NFD_path)$(NFD_Load)
CXXFLAGS =  -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lfftw3 -lnfd `pkg-config --cflags --libs gtk+-3.0`

all:	$(RES) run


run: $(MAIN)
	@read -p "Informe o arquivo de audio [mp3|wav|ogg] " AUD; \
	./$< $$AUD


$(MAIN):	$(MAINSRC)
			$(CXX) $< -o $@  $(NFD_cxx_flags) $(CXXFLAGS)

clean:
	@rm $(MAIN)
