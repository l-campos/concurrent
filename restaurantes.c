#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> 

#define MAX_ORDERS 10
#define READY_ORDERS 5
#define GARÇONS 2
#define COZINHEIROS 3
#define ENTREGADORES 2

// Buffer 1 (Garçom -> Cozinheiro): Fila de pedidos pendentes
int pending_orders[MAX_ORDERS];
int pending_count = 0;
int next_order_number = 1;

// Buffer 2 (Cozinheiro -> Entregador): Fila de pedidos prontos
int ready_orders[READY_ORDERS];
int ready_count = 0;

// Semáforos para o Buffer 1 (Pedidos Pendentes)
sem_t sem_pending_items; 
sem_t sem_pending_space; 
pthread_mutex_t mutex_pending = PTHREAD_MUTEX_INITIALIZER; 

// Semáforos para o Buffer 2 (Pedidos Prontos)
sem_t sem_ready_items;   
sem_t sem_space_ready;   
pthread_mutex_t mutex_ready = PTHREAD_MUTEX_INITIALIZER; 

/**
 * Função do Garçom (Produtor do Buffer 1)
 * 1. Espera por ESPAÇO na fila de pedidos pendentes.
 * 2. Trava o mutex, adiciona um pedido.
 * 3. Libera o mutex e sinaliza que há um novo ITEM.
 */

void* f_garcom(void* v) {
    int* id = (int*) v;
    while (1) {
        // Simula o tempo para anotar o pedido
        sleep(rand() % 2 + 1);

        // 1. Espera por um ESPAÇO LIVRE no balcão de pedidos
        sem_wait(&sem_pending_space);

        // 2. Trava o buffer para adicionar o pedido (seção crítica)
        pthread_mutex_lock(&mutex_pending);
        
        int order_num = next_order_number++;
        pending_orders[pending_count] = order_num;
        pending_count++;
        printf("Garçom %d anotou pedido %d. (Pendentes: %d)\n", *id, order_num, pending_count);
        
        pthread_mutex_unlock(&mutex_pending);

        // 3. Sinaliza que há um novo ITEM (pedido) disponível
        sem_post(&sem_pending_items);
    }
}

/**
 *  Função do Cozinheiro (Consumidor do Buffer 1, Produtor do Buffer 2)
 *
 * (Parte 1: Consumidor)
 * 1. Espera por um ITEM (pedido) na fila de pendentes.
 * 2. Trava o mutex, remove o pedido.
 * 3. Libera o mutex e sinaliza que há um novo ESPAÇO.
 *
 * (Parte 2: Produtor)
 * 4. Espera por ESPAÇO na fila de pedidos prontos.
 * 5. Trava o mutex, adiciona o pedido.
 * 6. Libera o mutex e sinaliza que há um novo ITEM.
 */
void* f_cozinheiro(void* v) {
    int* id = (int*) v;
    while (1) {
        
        // Consumidor do Buffer 1 (Pega pedido pendente) 

        // 1. Espera por um ITEM (pedido) no buffer de pedidos
        sem_wait(&sem_pending_items);

        // 2. Trava o buffer para remover o pedido (seção crítica)
        pthread_mutex_lock(&mutex_pending);
        
        pending_count--;
        int order = pending_orders[pending_count];
        printf("Cozinheiro %d pegou pedido %d. (Pendentes: %d)\n", *id, order, pending_count);
        
        pthread_mutex_unlock(&mutex_pending);

        // 3. Sinaliza que há um novo ESPAÇO LIVRE no buffer de pedidos
        sem_post(&sem_pending_space);


        // Simula preparação
        sleep(rand() % 3 + 2);

        // Produtor para o Buffer 2 (Coloca pedido pronto) 
        
        // 4. Espera por ESPAÇO no balcão de prontos
        sem_wait(&sem_space_ready);

        // 5. Trava o buffer 2 (seção crítica)
        pthread_mutex_lock(&mutex_ready);
        
        ready_orders[ready_count] = order;
        ready_count++;
        printf("Cozinheiro %d finalizou pedido %d. (Prontos: %d)\n", *id, order, ready_count);
        
        pthread_mutex_unlock(&mutex_ready);

        // 6. Avisa aos entregadores que há um ITEM pronto
        sem_post(&sem_ready_items);
    }
}

/**
 * Função do Entregador (Consumidor do Buffer 2)
 * 1. Espera por um ITEM (pedido pronto).
 * 2. Trava o mutex, remove o pedido.
 * 3. Libera o mutex e sinaliza que há um novo ESPAÇO.
 */
void* f_entregador(void* v) {
    int* id = (int*) v;
    while (1) {
        // 1. Espera por um ITEM (pedido pronto)
        sem_wait(&sem_ready_items);

        // 2. Trava o buffer 2 (seção crítica)
        pthread_mutex_lock(&mutex_ready);
        
        ready_count--;
        int order = ready_orders[ready_count];
        printf("Entregador %d retirou pedido %d. (Prontos: %d)\n", *id, order, ready_count);
        
        pthread_mutex_unlock(&mutex_ready);

        // 3. Avisa aos cozinheiros que há um ESPAÇO livre
        sem_post(&sem_space_ready);

        // Simula tempo de entrega
        sleep(rand() % 4 + 1);
    }
}

int main() {

    pthread_t garcom_thread[GARÇONS], cozinheiro_thread[COZINHEIROS], entregador_thread[ENTREGADORES];
    int garcom_id[GARÇONS] = {1, 2}, cozinheiro_id[COZINHEIROS] = {1, 2, 3}, entregador_id[ENTREGADORES] = {1, 2};

    srand(time(NULL));

    // Buffer 1 (Pedidos Pendentes)
    sem_init(&sem_pending_items, 0, 0);          
    sem_init(&sem_pending_space, 0, MAX_ORDERS); 

    // Buffer 2 (Pedidos Prontos)
    sem_init(&sem_ready_items, 0, 0);            
    sem_init(&sem_space_ready, 0, READY_ORDERS); 

    printf("Restaurante abrindo!\n");
    printf("Garçons: %d, Cozinheiros: %d, Entregadores: %d\n", GARÇONS, COZINHEIROS, ENTREGADORES);
    printf("Fila de Pedidos: %d, Balcão de Prontos: %d\n\n", MAX_ORDERS, READY_ORDERS);


    for (int i = 0; i < GARÇONS; i++) pthread_create(&garcom_thread[i], NULL, f_garcom, &garcom_id[i]);
    for (int i = 0; i < COZINHEIROS; i++) pthread_create(&cozinheiro_thread[i], NULL, f_cozinheiro, &cozinheiro_id[i]);
    for (int i = 0; i < ENTREGADORES; i++) pthread_create(&entregador_thread[i], NULL, f_entregador, &entregador_id[i]);

    for (int i = 0; i < GARÇONS; i++) pthread_join(garcom_thread[i], NULL);
    for (int i = 0; i < COZINHEIROS; i++) pthread_join(cozinheiro_thread[i], NULL);
    for (int i = 0; i < ENTREGADORES; i++) pthread_join(entregador_thread[i], NULL);
    
    return 0;
}
