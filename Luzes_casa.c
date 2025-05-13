/*
 * WebServer para controle de iluminação com RP2040
 * Autor: [Seu Nome]
 * Data: [Data]
 * 
 * Funcionalidades:
 * - Conexão Wi-Fi
 * - Servidor HTTP na porta 80
 * - Controle de matriz de LEDs WS2812 (5x5)
 * - Interface web intuitiva
 */

// ==================== BIBLIOTECAS ====================
#include <stdio.h>           // Para funções de entrada/saída (printf, etc.)
#include <string.h>          // Para manipulação de strings (strstr, memcpy, etc.)
#include <stdlib.h>          // Para alocação dinâmica (malloc, free)
#include "pico/stdlib.h"     // SDK básico da Pico (GPIO, sleep_ms, etc.)
#include "hardware/adc.h"    // Para leitura do ADC (temperatura interna)
#include "pico/cyw43_arch.h" // Driver Wi-Fi da Pico W
#include "lwip/pbuf.h"       // Buffers de rede (LWIP)
#include "lwip/tcp.h"        // Protocolo TCP (LWIP)
#include "lwip/netif.h"      // Interface de rede (LWIP)
#include "ws2812.pio.h"      // Para controle dos LEDs WS2812 via PIO

// ==================== DEFINIÇÕES ====================
// Credenciais da rede Wi-Fi
#define WIFI_SSID "DESKTOP-0G1AS8V 7081"    // Nome da rede Wi-Fi
#define WIFI_PASSWORD "12345678"             // Senha da rede

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN // LED interno da Pico W (GPIO do CYW43)
#define LED_BLUE_PIN 12               // GPIO para LED azul adicional
#define LED_GREEN_PIN 11              // GPIO para LED verde adicional
#define LED_RED_PIN 13                // GPIO para LED vermelho adicional

// Configuração da matriz de LEDs WS2812
#define WS2812_PIN 7  // Pino de dados da matriz
#define NUM_LEDS 25   // Número total de LEDs (5x5)
PIO pio = pio0;       // Controlador PIO (0 ou 1)
int sm = 0;           // Máquina de estado PIO (0-3)

// Estados dos cômodos (ligado/desligado)
bool sala = false;    // Estado da luz da sala
bool quarto1 = false; // Estado da luz do quarto 1
bool cozinha = false; // Estado da luz da cozinha
bool quarto2 = false; // Estado da luz do quarto 2

// ==================== PROTÓTIPOS DE FUNÇÕES ====================
void gpio_led_bitdog(void); // Configura GPIOs dos LEDs
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err); // Callback de aceitação de conexão
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err); // Callback de recebimento de dados
void user_request(char **request); // Processa requisições HTTP
void acender_leds(); // Atualiza matriz de LEDs
void put_pixel(uint32_t pixel_grb); // Envia cor para um LED
uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b); // Converte RGB para formato WS2812

// ==================== FUNÇÃO PRINCIPAL ====================
int main() {
    // Inicializa comunicação serial (USB)
    stdio_init_all();
    
    // Configura GPIOs dos LEDs adicionais
    gpio_led_bitdog();
    
    // Inicializa matriz de LEDs WS2812
    uint offset = pio_add_program(pio, &ws2812_program); // Carrega programa PIO
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false); // Configura hardware
    
    // Inicializa hardware Wi-Fi (CYW43)
    while (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1; // Sai se falhar
    }
    
    // Desliga LED interno (inicia desligado)
    cyw43_arch_gpio_put(LED_PIN, 0);
    
    // Configura modo station (conecta a rede Wi-Fi existente)
    cyw43_arch_enable_sta_mode();
    
    // Tenta conectar ao Wi-Fi
    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, 
           CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1; // Sai se falhar
    }
    printf("Conectado ao Wi-Fi\n");
    
    // Mostra IP atribuído ao dispositivo
    if (netif_default) {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }
    
    // Cria novo PCB (Protocol Control Block) TCP
    struct tcp_pcb *server = tcp_new();
    if (!server) {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }
    
    // Associa servidor à porta 80 (HTTP) em qualquer interface
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }
    
    // Coloca servidor em modo escuta
    server = tcp_listen(server);
    
    // Configura callback para conexões recebidas
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");
    
    // Inicializa ADC para leitura de temperatura (não usado neste exemplo)
    adc_init();
    adc_set_temp_sensor_enabled(true);
    
    // Loop principal
    while (true) {
        acender_leds();         // Atualiza matriz de LEDs
        cyw43_arch_poll();       // Mantém conexão Wi-Fi ativa
        sleep_ms(100);           // Pausa para reduzir uso da CPU
    }
    
    // Desliga Wi-Fi (nunca executado por causa do loop infinito)
    cyw43_arch_deinit();
    return 0;
}

// ==================== IMPLEMENTAÇÃO DAS FUNÇÕES ====================

// Configura GPIOs dos LEDs adicionais
void gpio_led_bitdog(void) {
    // LED Azul
    gpio_init(LED_BLUE_PIN);           // Inicializa pino
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT); // Configura como saída
    gpio_put(LED_BLUE_PIN, false);     // Inicia desligado
    
    // LED Verde
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);
    
    // LED Vermelho
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);
}

// Callback chamado quando nova conexão TCP é aceita
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    // Configura callback para quando dados forem recebidos nesta conexão
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK; // Retorna OK
}

// Processa requisições HTTP recebidas
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    // Se p é NULL, conexão foi fechada
    if (!p) {
        tcp_close(tpcb);     // Fecha conexão
        tcp_recv(tpcb, NULL); // Remove callback
        return ERR_OK;
    }
    
    // Aloca buffer para armazenar requisição
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len); // Copia dados recebidos
    request[p->len] = '\0';              // Adiciona terminador de string
    
    printf("Request: %s\n", request); // Log da requisição
    
    // Processa requisição (liga/desliga cômodos)
    user_request(&request);
    
    // Buffer para resposta HTML
    char html[1024];
    
    // Gera página HTML dinâmica
    snprintf(html, sizeof(html),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<!DOCTYPE html><html><head><title>Controle de Iluminacao</title><style>"
        "body{background-color:#004f5b;font-family:Arial,sans-serif;text-align:center;margin-top:50px;}"
        "h1{font-size:64px;margin-bottom:30px;color:white;}"
        "button{background-color:LightGray;font-size:20px;margin:10px;padding:20px 40px;border-radius:10px;width:400px;color:#0b0050;box-shadow:5px 5px 10px black;border:none;}"
        "button:hover{background-color:#008CBA;color:white;cursor:pointer;box-shadow:5px 5px 10px white;}.s{background-color:#7affa7;}"
        "</style></head><body>"
        "<h1>Controle de Iluminacao</h1>"
        "<form action=\"./sala_on\"><button class=\"%s\">%s Luz da Sala</button></form>"
        "<form action=\"./quarto1_on\"><button class=\"%s\">%s Luz do Quarto 1</button></form>"
        "<form action=\"./cozinha_on\"><button class=\"%s\">%s Luz da Cozinha</button></form>"
        "<form action=\"./quarto2_on\"><button class=\"%s\">%s Luz do Quarto 2</button></form>"
        "</body></html>",
        sala ? "s":"n", sala ? "Desligar" : "Ligar",  // Sala
        quarto1 ? "s":"n", quarto1 ? "Desligar" : "Ligar",  // Quarto1
        cozinha ?"s":"n", cozinha ? "Desligar" : "Ligar",  // Cozinha
        quarto2 ? "s":"n", quarto2 ? "Desligar" : "Ligar"  // Quarto2
    );
    
    // Envia resposta HTML
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb); // Força envio
    
    // Libera recursos
    free(request); // Libera buffer da requisição
    pbuf_free(p);  // Libera buffer de rede
    
    return ERR_OK;
}

// Processa requisições HTTP e altera estados dos cômodos
void user_request(char **request) {
    // Verifica qual botão foi pressionado
    if (strstr(*request, "GET /sala_on") != NULL) {
        sala = !sala; // Alterna estado da sala
    }
    else if (strstr(*request, "GET /quarto1_on") != NULL) {
        quarto1 = !quarto1; // Alterna estado do quarto1
    }
    else if (strstr(*request, "GET /cozinha_on") != NULL) {
        cozinha = !cozinha; // Alterna estado da cozinha
    }
    else if (strstr(*request, "GET /quarto2_on") != NULL) {
        quarto2 = !quarto2; // Alterna estado do quarto2
    }
}

// Envia cor para um LED da matriz
void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u); // Envia dados via PIO
}

// Converte valores RGB para formato esperado pelos WS2812
uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

// Atualiza todos os LEDs da matriz baseado nos estados dos cômodos
void acender_leds() {
    for (int i = 0; i < NUM_LEDS; ++i) {
        uint8_t r = 0, g = 0, b = 0; // Inicia com LED desligado

        // Sala (canto superior direito)
        if (sala && (i == 24 || i == 23 || i == 16 || i == 15)) {
            r = g = b = 5; // Branco suave
        }
        // Quarto1 (centro direito)
        else if (quarto1 && (i == 18 || i == 19 || i == 20 || i == 21)) {
            r = g = b = 5;
        }
        // Cozinha (canto inferior esquerdo)
        else if (cozinha && (i == 3 || i == 4 || i == 5 || i == 6)) {
            r = g = b = 5;
        }
        // Quarto2 (canto superior esquerdo)
        else if (quarto2 && (i == 0 || i == 1 || i == 8 || i == 9)) {
            r = g = b = 5;
        }
        // Cruz central (sempre vermelha)
        else if ((i % 5 == 2) || (i >= 10 && i <= 14)) {
            r = 10; g = 0; b = 0;
        }

        // Envia cor para o LED atual
        put_pixel(urgb_u32(r, g, b));
    }
}