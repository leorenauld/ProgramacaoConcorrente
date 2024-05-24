#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <math.h>

// Variáveis globais
sem_t slotCheio, slotVazio;  // semáforos para sincronização por condição
sem_t mutexGeral; //semaforo UNICO para sincronização entre produtores e consumidores e para log

int N;           // número de consumidores
int M;           // tamanho do buffer
FILE *file;      // arquivo binário

int primos = 0;
int primosReal = 0;
int *pontuacoes;
int *Buffer;

int terminou = 0; // flag para indicar que a produção terminou

// função para inserir um elemento no buffer
void Insere(int item) {
    static int in = 0;
    sem_wait(&slotVazio); // aguarda slot vazio para inserir
    sem_wait(&mutexGeral); // exclusão mútua entre produtores (aqui geral para log)
    Buffer[in] = item;
    in = (in + 1) % M;
    for (int i = 0; i < M; i++)
        printf("%d ", Buffer[i]);
    puts("");
    sem_post(&mutexGeral);
    sem_post(&slotCheio); // sinaliza um slot cheio
}

// função para retirar um elemento no buffer
int Retira(int id) {
    int item;
    static int out = 0;
    sem_wait(&slotCheio); // aguarda slot cheio para retirar
    sem_wait(&mutexGeral); // exclusão mútua entre consumidores (aqui geral para log)
    item = Buffer[out];
    Buffer[out] = 0;
    out = (out + 1) % M;
    printf("Cons[%d]: retirou %d\n", id, item);
    for (int i = 0; i < M; i++)
        printf("%d ", Buffer[i]);
    puts("");
    sem_post(&mutexGeral);
    sem_post(&slotVazio); // sinaliza um slot vazio
    return item;
}

int ehPrimo(long long int n) {
    int i;
    if (n <= 1) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (i = 3; i <= sqrt(n); i += 2)
        if (n % i == 0) return 0;
    return 1;
}

// função que testa se o buffer tem algum número não processado para condição de parada das threads consumidoras
int bufferCheio(int *vetor, int tamanho) {
    for (int i = 0; i < tamanho; i++) {
        if (vetor[i] != 0) {
            return 1; // retorna 1 se encontrar um elemento diferente de zero
        }
    }
    return 0; // retorna 0 se todos os elementos forem zero
}

void *produtor(void *arg) {
    int id = *(int *)(arg);
    free(arg);
    int aux = 0;
    int num;
    while (fread(&num, sizeof(int), 1, file) == 1) {
        if (aux > num) {
            primosReal = num;
            break;
        }
        Insere(num);
        aux = num;
    }
    sem_wait(&mutexGeral);
    terminou = 1;
    sem_post(&mutexGeral);
    for (int i = 0; i < N; i++) {
        sem_post(&slotCheio); 
    }
    pthread_exit(NULL);
}

void *consumidor(void *arg) {
    int item, id = *(int *)(arg);
    int pontuacao = 0;
    free(arg);
    while (1) {
        sem_wait(&mutexGeral);
        if (terminou && !bufferCheio(Buffer, M)) {
            sem_post(&mutexGeral);
            break;
        }
        sem_post(&mutexGeral);
        item = Retira(id);
        if (ehPrimo(item)) {
            primos++;
            pontuacao++;
        }
    }
    pontuacoes[id] = pontuacao;
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <N> <M> <arquivo>\n", argv[0]);
        return 1;
    }

    N = atoi(argv[1]);
    M = atoi(argv[2]);
    file = fopen(argv[3], "rb");

    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    // Alocar memória para os vetores globais
    Buffer = (int *)malloc(M * sizeof(int));
    pontuacoes = (int *)malloc(N * sizeof(int));

    pthread_t *tid = (pthread_t *)malloc((N + 1) * sizeof(pthread_t));

    // Inicia os semáforos
    sem_init(&mutexGeral, 0, 1); // binário
    sem_init(&slotCheio, 0, 0); // contador
    sem_init(&slotVazio, 0, M); // contador

    // Inicia a thread produtora
    int *idProdutor = malloc(sizeof(int));
    *idProdutor = 0;
    if (pthread_create(&tid[0], NULL, produtor, idProdutor)) {
        printf("Erro na criação do thread produtor\n");
        exit(1);
    }

    // Inicia as threads consumidoras
    for (int i = 0; i < N; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        if (pthread_create(&tid[1 + i], NULL, consumidor, id)) {
            printf("Erro na criação do thread consumidor\n");
            exit(1);
        }
    }

    // Espera as threads terminarem
    for (int i = 0; i < N + 1; i++) {
        if (pthread_join(tid[i], NULL)) {
            printf("Erro no join\n");
        }
    }

    // testa se o resultado está correto e imprime a thread vencedora
    if (primos == primosReal) {
        printf("Foram encontrados, corretamente, %d primos.\n", primos);

        int aux = 0;
        int maior = 0;
        for (int i = 0; i < N; i++) {
            if (pontuacoes[i] > aux) {
                maior = i;
                aux = pontuacoes[i];
            }
        }
        printf("A thread %d ganhou com %d primos encontrados.\n", maior, aux);
    }

    free(Buffer);
    free(pontuacoes);
    free(tid);
    fclose(file);

    pthread_exit(NULL);
    return 0;
}
