# 💡 WebServer de Controle de Iluminação com Raspberry Pi Pico W

Este projeto implementa um sistema de automação residencial simples, permitindo o **controle remoto de iluminação** de diferentes cômodos por meio de uma **interface web**, utilizando o microcontrolador **Raspberry Pi Pico W** e a placa **BitDogLab**.

## 📱 Funcionalidades

- Conexão Wi-Fi com redes WPA2.
- Servidor Web embutido (porta 80).
- Interface web intuitiva com botões para controle de cômodos.
- Visualização dos cômodos acesos através de **matriz de LEDs WS2812 (5x5)**.

## 🧠 Visão Geral

A partir de qualquer dispositivo conectado à mesma rede Wi-Fi, o usuário pode acessar uma página web hospedada pelo próprio microcontrolador e acionar luzes virtuais dos cômodos: **Sala**, **Quarto 1**, **Cozinha** e **Quarto 2**. A ativação é refletida na **matriz de LEDs**, e os estados são alternados dinamicamente com atualização visual.

## 🖼️ Interface Web

A página web gerada pelo sistema apresenta botões grandes e responsivos, cada um indicando o estado atual (ligado/desligado) da luz em um cômodo:

![image](https://github.com/user-attachments/assets/5998d701-ed2f-4e93-bdce-48135585f0e5)


## 🔌 Periféricos Utilizados

| Componente               | Uso no Projeto                                           |
|--------------------------|----------------------------------------------------------|
| **Wi-Fi (CYW43)**        | Conexão com rede local para hospedar o servidor web      |
| **Matriz de LEDs WS2812**| Exibição gráfica dos cômodos acesos/apagados            |
| **LEDs RGB (GPIO 11-13)**| Representação visual adicional dos estados              |
| **GPIOs**                | Controle dos LEDs externos                               |
| **PIO**                  | Controle preciso da matriz WS2812                        |
| **Servidor TCP (LWIP)**  | Recepção e resposta às requisições HTTP                  |

## ⚙️ Requisitos

- Placa **Raspberry Pi Pico W**
- Placa **BitDogLab** (com matriz WS2812 conectada no GPIO 7)
- SDK Pico instalado com suporte a LWIP e PIO
- Compilador CMake/GCC compatível


## 🚀 Como Usar

1. Clone o repositório e compile com a **Pico-SDK**.
2. Configure o **SSID** e a **senha da sua rede Wi-Fi** no `main.c`.
3. Faça o upload do binário na Pico W.
4. Após iniciar, conecte-se ao mesmo Wi-Fi e acesse o IP exibido no terminal.
5. Controle a iluminação virtual via navegador!

## 📅 Autor

- Bruna Alves de Barros - Polo Bom Jesus da Lapa – Maio/2025



