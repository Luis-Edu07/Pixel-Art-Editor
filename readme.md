# Pixel Art Editor

Editor de pixel art multiplataforma feito em C++17 e Qt 6. Inclui desenho, seleções, camadas, animação por quadros e exportação de PNG, GIF e sprite sheets.

## Baixar uma versão pronta

Abra a página **Releases** deste repositório e baixe o arquivo adequado ao seu sistema:

- **Windows 64 bits:** `pixel-art-editor-windows-x64.zip`. Extraia o ZIP e abra `bin/PixelArtEditor.exe`.
- **Linux 64 bits:** `pixel-art-editor-linux-x64.tar.gz`. Extraia o arquivo e execute `./bin/PixelArtEditor`.

Os pacotes incluem as bibliotecas Qt usadas pelo aplicativo. O pacote Linux é compilado no Ubuntu 24.04 e pode depender de bibliotecas do sistema compatíveis com essa base.

Cada nova versão publicada como uma tag `v*.*.*` gera os dois pacotes e os anexa automaticamente a uma Release. O workflow também compila e roda os testes a cada push na branch `main`; nesses casos, os pacotes ficam em **Actions → execução → Artifacts**.

## Compilar a partir do código

Requisitos: CMake 3.21+, compilador C++17 e Qt 6.2+ com os módulos Widgets e Gui.

```bash
cmake -S . -B build
cmake --build build --config Release
```

Para criar uma pasta instalável com as dependências Qt (Qt 6.3+):

```bash
cmake --install build --config Release --prefix package
```

No Windows com Visual Studio, o executável fica em `package/bin/PixelArtEditor.exe`. No Linux, fica em `package/bin/PixelArtEditor`.

## Testes

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Publicar uma versão

Envie uma tag semântica para iniciar o build dos pacotes e criar uma Release:

```bash
git tag v1.0.0
git push origin v1.0.0
```
