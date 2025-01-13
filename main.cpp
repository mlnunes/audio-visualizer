#include <SFML/Audio/Sound.hpp>
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <SFML/Audio.hpp>
#include <cstddef>
#include <fftw3.h>
#include <cstdlib>
#include <memory>
#include <iostream>
#include <cmath>
#include <string>

int main (int argc, char** argv){
  sf::Color gray(128, 128, 128); //Define uma cor cinza de intensidade média
  
  //Inicializa uma janela através da criação de um ponteiro inteligente
  auto window = std::make_unique<sf::RenderWindow>(
    sf::VideoMode(1280, 720),
    "Espectro de Audio",
    sf::Style::Titlebar | sf::Style::Close
  );
  window->setFramerateLimit(30);
  sf::Vector2u windowSize = window->getSize();
  auto [winWidth, winHeight] = windowSize;

  //Verifica se foi passado o nome do arquivo de audio
  if (argc < 2){
    std::cerr << "Modo de uso: " << argv[0] << "arquivo.[mp3|wav|ogg]" << std::endl;
    return EXIT_FAILURE;
  }
  
  //Verifica se o arquivo existe
  sf::SoundBuffer buffer;
  if(!buffer.loadFromFile(argv[1])){
    std::cerr << "Falha ao carregar o arquivo " << argv[1] << std::endl;
    return EXIT_FAILURE;
  }

  //Cria o texto com o nome do arquivo sem a extensão para ser exibido no canto superior direito
  sf::Font font;
  font.loadFromFile("./NotoSansMath-Regular.ttf");
  std::string music = argv[1];
  size_t pos = music.find_last_of('.');
  if (pos != std::string::npos) {
    music = music.substr(0, pos);
  }
  sf::Text text(music, font, 18);
  text.setPosition(20.f, 10.f);


  //Cria um slide para controlar o volume do audio
  sf::RectangleShape slideBar (sf::Vector2f(400.f, 10.f));
  slideBar.setFillColor(sf::Color::White);
  slideBar.setPosition( 440.f, 650.f);

  //Indicado do valor do slider
  sf::RectangleShape sliderKnob(sf::Vector2f(10.f, 30.f));
  sliderKnob.setFillColor(gray);
  sliderKnob.setPosition(440.f + (400.f * 0.5f), 640.f); //posiciona no meio da janela


  //cria o objeto sound para carregar o audio inteiro na memória e reproduzi-lo
  sf::Sound sound(buffer);
  sound.setVolume(50.f);
  sound.play();
  
  //obtém o tempo de duração total da música
  sf::Time duration = buffer.getDuration();

  //Cria uma barra para representar a duração total do audio
  sf::RectangleShape durationBar (sf::Vector2f(400.f, 10.f));
  durationBar.setFillColor(sf::Color::White);
  durationBar.setPosition(440.f, 550.f);

  sf::Time  currentOffset = sound.getPlayingOffset();
  float offsetWidth = (currentOffset.asSeconds() / duration.asSeconds()) * 400.f;
  sf::RectangleShape offsetBar (sf::Vector2f(offsetWidth, 10.f));
  offsetBar.setFillColor(gray);
  offsetBar.setPosition(440.f, 550.f);

 
  const int sample_size = 1024;
  //cria os vetores complexos, dois elementos um real e outro imaginário para amazenar
  //o resultado da fft
  //FFTW_FORWARD indica conversão do domínio do tempo para frequencia
  fftw_complex *in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * sample_size);
  fftw_complex *out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * sample_size);

  //planeja a execução de uma transformada discreta de Fourier(1D) no vetor de entrada in
  //e salva o resultado no vetor de saida out
  fftw_plan plan = fftw_plan_dft_1d(sample_size, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
  
  //cria o vetor para armazenar a fft das frequências positivas
  std::vector<float> espectro(sample_size / 2);
  
  //acessando os dados de audio
  const sf::Int16 *samples = buffer.getSamples(); //obtém o ponteiro paras as amplitudes de audio amazenado no sf::SoundBuffer
  std::size_t sample_count = buffer.getSampleCount(); //armazena o numero de amostras no buffer
  std::size_t channels = buffer.getChannelCount(); //armazena o número de canais (mono, estéreo, ...)
  std::size_t current_sample = {};
  

  //Inicializa o loop pricipal de execução
  bool isDragging = false; // Rastreamento do estado do clique
  while(window->isOpen()){
    //carrega os controles da janela
    auto event = std::make_unique<sf::Event>();
    while (window->pollEvent(*event)) {
      if (event->type == sf::Event::Closed){
          window->close();
      }

      //Clique no mouse para iniciar o controle
      if (event->type == sf::Event::MouseButtonPressed && event->mouseButton.button == sf::Mouse::Left){
        if (sliderKnob.getGlobalBounds().contains(event->mouseButton.x, event->mouseButton.y)){
          isDragging = true;
        }
      } 
      
      //Soltar o botão do mouse para parar o controle
      if (event->type == sf::Event::MouseButtonReleased && event->mouseButton.button == sf::Mouse::Left){
        isDragging = false;
      }
    }

    //Arrastar o slider
    if (isDragging){
      sf::Vector2i mousePos = sf::Mouse::getPosition(*window);
      float knobX = std::clamp(static_cast<float>(mousePos.x), slideBar.getPosition().x,
                    slideBar.getPosition().x + slideBar.getSize().x - sliderKnob.getSize().x);
      sliderKnob.setPosition(knobX, sliderKnob.getPosition().y);

      //Atualiza o volume com base na posição do knob
      float relativePosition = (knobX - slideBar.getPosition().x) / slideBar.getSize().x;
      sound.setVolume(relativePosition * 100.f);
    }
 
    //encerra a janela se o audio terminar
    if (sound.getStatus() == sf::Sound::Stopped){
      break;
    }

    for (int i = 0; i < sample_size; ++i ){
      std::size_t index = (current_sample + i)% (sample_count / channels);
      in[i][0] = samples[index * channels] / 32768.0; //armazena os valores de amplitude dos samples de um canal (caso tenha mais de um) de forma normalizada (/32768 ou 2^16)
      in[i][1] = 0.0;
    }

    //aplica o plano de execução da fftw (plan) nos dados contidos em in e armazena o resultado em out
    fftw_execute(plan);

    for (int i = {}; i < sample_size / 2; ++i){
      espectro[i] = std::sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]); //calcula a magnitude de cada caomponente de frequência
    }

    // Exibir a duração em segundos
    current_sample += sample_size;

    window->clear();

    //Cria e exibe as barras do espectro no lado direito
    for (int i = {}; i < 50; i++){
      //cria a barra
      sf::RectangleShape bar;
      //atribui o tamanho conforme a magnitude
      bar.setSize(sf::Vector2f(2, espectro[i] * 1.f));
      //posiciona a barra na parte superior
      bar.setPosition(
        i * 6 + winWidth / 2.f,
        0.6f * winHeight);
      bar.setRotation(180);
      window->draw(bar);
      //repete a barra na parte inferior
      bar.setSize(sf::Vector2f(2, -(espectro[i] * 1.f)));
      window->draw(bar);
    }

    //redesenha as barras no lado esquerdo
    for (int i = 49; i >= 0; --i){
      sf::RectangleShape bar;
      bar.setSize(sf::Vector2f(2, espectro[i] * 1.f));
      bar.setPosition(
        (49 - i) * 6 + 0.27f * winWidth,
        0.6f * winHeight);
      bar.setRotation(180);
      window->draw(bar);
      bar.setSize(sf::Vector2f(2, -(espectro[i] * 1.f)));
      window->draw(bar);
    }

    //atualiza a barra de duração
    currentOffset = sound.getPlayingOffset();
    offsetWidth = (currentOffset.asSeconds() / duration.asSeconds()) * 400.f;
    offsetBar.setSize(sf::Vector2f(offsetWidth, 10.f));

    window->draw(text);
    window->draw(slideBar);
    window->draw(sliderKnob);
    window->draw(durationBar);
    window->draw(offsetBar);
    window->display();
  }

  //libera a memória
  fftw_destroy_plan(plan);
  fftw_free(in);
  fftw_free(out);

  //encerra
  return EXIT_SUCCESS;
}
