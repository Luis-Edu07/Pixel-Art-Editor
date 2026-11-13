🎨 Pixel Art Editor

Um editor multiplataforma de Pixel Art, Sprites, Animações e Tilesets, desenvolvido com foco em simplicidade, desempenho e compatibilidade com ferramentas de desenvolvimento de jogos.

O projeto tem como objetivo oferecer uma alternativa leve e extensível para criação de assets 2D, permitindo desenhar pixel a pixel, criar animações, montar tilesets, gerar sprite sheets e exportar os recursos para utilização em diferentes engines e projetos.

---

📌 Objetivos

O Pixel Art Editor pretende oferecer:

- 🎨 Criação e edição de Pixel Art
- 🖼️ Criação de sprites
- 🎞️ Animação quadro a quadro
- 🧱 Criação e edição de Tilesets
- 📦 Geração de Sprite Sheets
- 🎨 Sistema de paletas
- 🗂️ Sistema de camadas
- ↩️ Undo/Redo
- 🔍 Zoom com preservação dos pixels
- 📤 Exportação para diferentes formatos
- 💾 Formato próprio de projeto
- 🧩 Exportação de metadados para utilização em engines
- 🖥️ Suporte a Windows e Linux

---

🖥️ Plataformas

O projeto será desenvolvido com suporte multiplataforma desde o início.

Windows

Distribuição planejada:

PixelArtEditor.exe

ou através de instalador:

PixelArtEditor-Setup.exe

Linux

Possíveis formatos de distribuição:

PixelArtEditor.AppImage

pixelart-editor.deb

Futuramente:

Flatpak

---

🛠️ Tecnologias

Linguagem principal

C++

A escolha do C++ tem como objetivo oferecer:

- Alto desempenho
- Baixo consumo de memória
- Controle preciso sobre os dados dos pixels
- Boa integração com APIs gráficas
- Facilidade para trabalhar com arquivos binários
- Excelente suporte multiplataforma

---

Interface gráfica

Qt 6

O Qt será utilizado para:

- Interface gráfica
- Janelas
- Menus
- Botões
- Painéis
- Diálogos
- Gerenciamento de eventos
- Renderização do editor
- Entrada de mouse e teclado

---

Sistema de build

CMake

O CMake será utilizado para permitir que o mesmo projeto seja compilado em diferentes sistemas operacionais.

Estrutura:

CMake
│
├── Windows
│
└── Linux

---

Controle de versão

Git

O projeto será versionado utilizando Git.

Repositório recomendado:

GitHub

---

🏗️ Arquitetura

A aplicação será dividida em módulos para evitar que a interface gráfica fique diretamente acoplada à lógica do editor.

PixelArtEditor/
│
├── src/
│ │
│ ├── core/
│ │ ├── Canvas
│ │ ├── Pixel
│ │ ├── Layer
│ │ ├── Selection
│ │ └── Project
│ │
│ ├── editor/
│ │ ├── CanvasView
│ │ ├── Tools
│ │ ├── Timeline
│ │ └── TilesetEditor
│ │
│ ├── tools/
│ │ ├── Pencil
│ │ ├── Eraser
│ │ ├── Fill
│ │ ├── Line
│ │ ├── Rectangle
│ │ ├── Select
│ │ └── ColorPicker
│ │
│ ├── animation/
│ │ ├── Frame
│ │ └── Animation
│ │
│ ├── formats/
│ │ ├── PNG
│ │ ├── GIF
│ │ ├── JSON
│ │ └── Project
│ │
│ └── main.cpp
│
├── assets/
│
├── tests/
│
├── docs/
│
├── CMakeLists.txt
│
└── README.md

---

🎨 Canvas

O Canvas será o núcleo do editor.

Cada imagem será representada como uma matriz de pixels.

Exemplo:

16 × 16 pixels

Internamente:

struct Pixel {
uint8_t r;
uint8_t g;
uint8_t b;
uint8_t a;
};

O Canvas poderá armazenar os pixels através de uma estrutura semelhante a:

class Canvas {
public:
int width;
int height;

    std::vector<Pixel> pixels;

};

Isso permitirá manipulação individual de cada pixel.

---

🔍 Sistema de Zoom

O editor deverá trabalhar com diferentes níveis de zoom:

25%
50%
100%
200%
400%
800%
1600%
3200%

O zoom deverá utilizar Nearest Neighbor para preservar a aparência da Pixel Art.

Exemplo:

Original:

██
██

Com zoom:

████████
████████
████████
████████

Não deverá ocorrer suavização ou interpolação dos pixels.

---

🟦 Grade de Pixels

Quando o zoom estiver suficientemente alto, o usuário poderá ativar uma grade:

┌─┬─┬─┬─┐
│█│█│ │ │
├─┼─┼─┼─┤
│█│ │█│ │
├─┼─┼─┼─┤
│ │█│ │█│
└─┴─┴─┴─┘

Opções:

☑ Mostrar grade
☑ Encaixar no pixel
☑ Mostrar coordenadas

---

🖌️ Ferramentas

Ferramentas básicas

O editor deverá possuir inicialmente:

- ✏️ Lápis
- 🧽 Borracha
- 🪣 Balde de preenchimento
- 📏 Linha
- ▭ Retângulo
- ⭕ Elipse
- 🖱️ Conta-gotas
- ✋ Pan
- 🔍 Zoom

---

🔲 Seleção

O sistema de seleção deverá permitir:

- Seleção retangular
- Seleção livre
- Copiar
- Recortar
- Colar
- Mover
- Redimensionar
- Espelhar horizontalmente
- Espelhar verticalmente
- Rotacionar 90°

---

↩️ Undo / Redo

Todas as operações importantes deverão ser registradas em um histórico.

Exemplo:

Desenhar pixel
↓
Desenhar linha
↓
Preencher área
↓
Apagar

O usuário poderá utilizar:

Ctrl + Z

para desfazer operações.

E:

Ctrl + Shift + Z

ou:

Ctrl + Y

para refazer.

---

🎨 Sistema de Paletas

O editor deverá possuir um sistema de paletas personalizadas.

Exemplo:

┌─────────────────────┐
│ PALETA │
├─────────────────────┤
│ ■ ■ ■ ■ ■ ■ ■ ■ │
│ ■ ■ ■ ■ ■ ■ ■ ■ │
└─────────────────────┘

Funcionalidades:

- Adicionar cor
- Remover cor
- Editar cor
- Selecionar cor
- Substituir cor
- Salvar paleta
- Carregar paleta

Formatos planejados:

.gpl
.json

---

🗂️ Camadas

O editor deverá suportar múltiplas camadas.

Exemplo:

Layers

☑ Effects
☑ Hair
☑ Body
☑ Clothes
☑ Background

Cada camada poderá possuir:

- Nome
- Visibilidade
- Opacidade
- Ordem
- Bloqueio

---

🎞️ Sistema de Animação

O editor deverá permitir animações quadro a quadro.

Exemplo:

Animation: Walk

FPS: 12

Frame 1
Frame 2
Frame 3
Frame 4
Frame 5
Frame 6

Timeline:

0 1 2 3 4 5
│ │ │ │ │ │
█ █ █ █ █ █

Funcionalidades:

- Criar frames
- Excluir frames
- Duplicar frames
- Reordenar frames
- Definir FPS
- Preview da animação
- Loop
- Onion Skin

---

🧱 Tilesets

O editor deverá possuir um modo específico para criação de Tilesets.

Exemplo:

Tile Size:

Width: 16
Height: 16

Também deverá suportar:

16 × 16
32 × 32
64 × 64

ou dimensões personalizadas.

---

🧩 Editor de Tileset

Uma imagem poderá ser automaticamente dividida em tiles.

Exemplo:

┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ 4 │
├───┼───┼───┼───┤
│ 5 │ 6 │ 7 │ 8 │
├───┼───┼───┼───┤
│ 9 │10 │11 │12 │
└───┴───┴───┴───┘

Recursos disponíveis:

- Tile IDs
- Tags
- Collision
- Terrain
- Layers
- Metadata
- Exportação de um tile selecionado como PNG

O projeto armazena tags e máscaras de vizinhança e consegue localizar uma variante de autotiling. O cálculo automático das máscaras e a pintura de terreno por regras ainda são futuros.

---

📦 Sprite Sheets

O programa deverá conseguir transformar animações em Sprite Sheets.

Exemplo:

Frame 1
Frame 2
Frame 3
Frame 4
Frame 5
Frame 6

Resultado:

┌────┬────┬────┬────┬────┬────┐
│ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │
└────┴────┴────┴────┴────┴────┘

Configurações:

Padding: 2 px
Margin: 4 px
Columns: 6
Rows: 1

---

📤 Exportação

Imagens

Formatos planejados:

PNG
BMP
GIF

PNG será o formato principal para assets de jogos.

---

Sprite Sheets

O programa deverá exportar:

sprite_sheet.png

e opcionalmente:

sprite_sheet.json

---

📄 Exportação de Metadados

Os sprites poderão ser acompanhados de informações em JSON.

Exemplo:

{
"sprite": "player",
"width": 32,
"height": 32,
"frames": [
{
"x": 0,
"y": 0,
"width": 32,
"height": 32
},
{
"x": 32,
"y": 0,
"width": 32,
"height": 32
}
]
}

Isso permitirá integração com:

- Godot
- Unity
- GameMaker
- Engines próprias
- Jogos em C++
- Jogos em Python

---

💾 Formato de Projeto

O programa deverá possuir um formato próprio.

Exemplo:

player.pxl

O formato poderá armazenar:

- Canvas
- Pixels
- Camadas
- Paletas
- Animações
- Frames
- Tilesets
- Configurações
- Metadados

Exemplo conceitual:

{
"version": 1,

    "canvas": {
        "width": 64,
        "height": 64
    },

    "layers": [
        {
            "name": "Body",
            "visible": true
        },
        {
            "name": "Eyes",
            "visible": true
        }
    ],

    "animations": [
        {
            "name": "Idle",
            "fps": 8
        }
    ]

}

A estrutura definitiva poderá utilizar um formato binário para reduzir o tamanho dos arquivos e melhorar o desempenho.

---

🖥️ Interface

A interface inicial poderá seguir uma estrutura semelhante a:

┌───────────────────────────────────────────────────────────┐
│ Arquivo Editar Imagem Camada Animação Ajuda │
├──────┬──────────────────────────────────────────┬─────────┤
│ │ │ │
│ ✏️ │ │ CORES │
│ 🧽 │ │ │
│ 🪣 │ CANVAS │ ■ ■ ■ │
│ 📏 │ │ ■ ■ ■ │
│ ▭ │ │ │
│ ⭕ │ │ LAYERS │
│ 🖱️ │ │ │
│ │ │ Body │
├──────┴──────────────────────────────────────────┴─────────┤
│ Zoom: 800% Canvas: 64 × 64 px │
└───────────────────────────────────────────────────────────┘

A interface poderá evoluir posteriormente para temas:

Dark
Light
Custom

---

🏗️ Desenvolvimento por etapas

O desenvolvimento será dividido em versões.

v0.1 — MVP

Primeira versão funcional.

- [x] Criar Canvas
- [x] Abrir imagem
- [x] Salvar projeto
- [x] Exportar PNG
- [x] Lápis
- [x] Borracha
- [x] Balde
- [x] Conta-gotas
- [x] Zoom e pan
- [x] Grade
- [x] Paleta
- [x] Undo/Redo

---

v0.2 — Camadas

- [x] Sistema de Layers
- [x] Criar camada
- [x] Excluir camada
- [x] Renomear camada
- [x] Ocultar camada
- [x] Opacidade
- [x] Bloquear camada

---

v0.3 — Seleção

- [x] Seleção retangular
- [x] Seleção livre
- [x] Copiar
- [x] Recortar
- [x] Colar
- [x] Mover
- [x] Espelhar
- [x] Rotacionar

---

v0.4 — Sprites

- [x] Criar sprites
- [x] Gerenciar frames
- [x] Sprite Sheet
- [x] Exportação de sprites
- [x] JSON de metadados

---

v0.5 — Animação

- [x] Timeline
- [x] Frames
- [x] FPS
- [x] Loop
- [x] Preview
- [x] Onion Skin
- [x] Exportação GIF

---

v0.6 — Tilesets

- [x] Editor de Tileset
- [x] Definição de tamanho
- [x] Grid
- [x] Tile IDs
- [x] Tags
- [x] Metadata
- [x] Exportar tile selecionado como PNG

---

v0.7 — Recursos avançados

- [ ] Auto-tiling
- [x] Terrain (tags e metadados)
- [x] Collision
- [x] Importação de paletas `.gpl` e `.json`
- [ ] Mais formatos
- [x] Atalhos personalizáveis

---

v1.0 — Versão estável

Objetivo:

«Um editor completo de Pixel Art, Sprites, Animações e Tilesets pronto para utilização em projetos reais de jogos.»

---

🧪 Testes

O projeto deverá possuir testes automatizados para as principais partes do sistema.

Exemplos:

tests/
├── CanvasTest
├── PixelTest
├── LayerTest
├── SelectionTest
├── AnimationTest
├── TilesetTest
└── ExportTest

Os testes deverão verificar principalmente:

- Manipulação de pixels
- Redimensionamento
- Camadas
- Seleções
- Animações
- Exportação
- Importação
- Integridade dos arquivos ".pxl"

---

⚙️ Compilação

Requisitos

Windows

- Windows 10 ou superior
- Qt 6
- CMake
- Compilador C++

Podendo utilizar:

Visual Studio

ou:

MinGW

---

Linux

Distribuições modernas deverão ser suportadas.

Requisitos:

Qt 6
CMake
GCC ou Clang

Exemplo:

sudo apt install build-essential cmake qt6-base-dev

---

🔨 Build

Clone o projeto:

git clone https://github.com/SEU-USUARIO/pixel-art-editor.git

Entre no diretório:

cd pixel-art-editor

Crie o diretório de build:

mkdir build
cd build

Configure:

cmake ..

Compile:

cmake --build .

---

📦 Distribuição

O projeto deverá futuramente possuir processos automatizados de build.

Exemplo:

GitHub Actions
│
├── Windows
│ └── .exe
│
└── Linux
├── AppImage
└── .deb

Isso permitirá gerar versões automaticamente a cada release.

---

🎮 Integração com Game Engines

O objetivo não é apenas criar imagens, mas facilitar a utilização dos assets em jogos.

Possíveis integrações:

Godot

PNG
JSON
Sprite Sheet
Tileset

Unity

PNG
Sprite Sheet
JSON

GameMaker

PNG
Sprite Sheet

Engine própria

PNG
JSON
Formato próprio

---

🧠 Princípios do projeto

O desenvolvimento deverá seguir alguns princípios:

1. Pixel Perfect

O editor nunca deverá alterar pixels involuntariamente.

2. Não destrutivo

Sempre que possível, operações deverão poder ser desfeitas.

3. Multiplataforma

Windows e Linux serão considerados desde o início.

4. Modularidade

Cada recurso deverá possuir baixo acoplamento com os demais módulos.

5. Desempenho

O editor deverá continuar responsivo mesmo trabalhando com projetos maiores.

6. Extensibilidade

Novas ferramentas e formatos deverão poder ser adicionados sem reescrever o núcleo da aplicação.

---

🗺️ Visão futura

Após a versão 1.0, poderão ser adicionados:

- 🌐 Suporte a projetos colaborativos
- ☁️ Sincronização
- 🧩 Sistema de plugins
- 🎮 Integração direta com engines
- 🗺️ Editor de mapas
- 🧱 Auto-tiling avançado
- 🎨 Gerador de paletas
- 🤖 Ferramentas assistidas por IA
- 📐 Guias e réguas
- 🖌️ Brushes personalizados
- 📚 Biblioteca de assets
- 🔄 Histórico avançado
- ⌨️ Atalhos configuráveis

---

📁 Estrutura final planejada

PixelArtEditor/
│
├── src/
│ ├── core/
│ ├── editor/
│ ├── tools/
│ ├── animation/
│ ├── tileset/
│ ├── formats/
│ └── main.cpp
│
├── assets/
├── tests/
├── docs/
│
├── CMakeLists.txt
├── README.md
├── LICENSE
└── .gitignore

---

📜 Licença

A licença do projeto ainda será definida.

Possíveis opções:

MIT
GPL-3.0
Apache-2.0

---

🚧 Status

«🟡 MVP funcional; recursos avançados em desenvolvimento»

O aplicativo em C++/Qt 6 já oferece edição de sprites em camadas, seleção, animação por frames, projetos `.pxl`, exportação PNG/BMP/GIF, sprite sheets e edição de metadados de tilesets. A exportação de tiles individuais também está disponível.

### Lacunas identificadas

- O autotiling ainda não calcula máscaras nem posiciona tiles automaticamente; o projeto armazena tags/máscaras e permite localizar uma variante correspondente.
- O editor não oferece reorganização/recorte de tiles dentro do atlas nem pintura de terrenos baseada em regras.
- Os testes automatizados cobrem o modelo e o formato `.pxl`, mas ainda não exercitam a interface, ferramentas do canvas, seleção ou timeline.
- Temas, brushes personalizados, formatos adicionais e integração direta com engines continuam no roadmap.

### Compilação local

Requisitos: CMake 3.21+, compilador C++17 e Qt 6 (módulos Widgets e Gui).

```sh
cmake -S . -B build
cmake --build build
```

Esses comandos compilam o aplicativo localmente. Nenhum executável foi gerado nesta etapa.

---

🎯 Objetivo final

O objetivo do projeto é criar uma ferramenta completa para desenvolvimento de assets 2D:

                    Pixel Art Editor
                           │
          ┌────────────────┼────────────────┐
          │                │                │
       Pixel Art         Sprites         Tilesets
          │                │                │
          └────────────────┼────────────────┘
                           │
                       Animações
                           │
                     Sprite Sheets
                           │
                     Exportação
                           │
          ┌────────────────┼────────────────┐
          │                │                │
        Godot            Unity          GameMaker
          │                │                │
          └────────────────┼────────────────┘
                           │
                         Jogos

A proposta é que o usuário consiga sair de uma tela vazia e chegar ao asset final pronto para ser utilizado em um jogo, utilizando uma única ferramenta.
