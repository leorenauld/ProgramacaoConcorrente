#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int ehPrimo(long long int n) {
    int i;
    if (n <= 1) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (i = 3; i <= sqrt(n); i += 2)
        if (n % i == 0) return 0;
    return 1;
}

int main(int argc, char *argv[]) {
    FILE *file;
    int n, countPrimos = 0;

    // Verifica se o número correto de argumentos foi fornecido
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <n>\n", argv[0]);
        return 1;
    }

    // Converte o argumento fornecido para inteiro
    n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Erro: <n> deve ser um número inteiro positivo.\n");
        return 1;
    }

    // Abre o arquivo binário para escrita
    file = fopen("numeros.bin", "wb");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    // Escreve os números de 1 até n no arquivo binário e conta os primos
    for (int i = 1; i <= n; i++) {
        fwrite(&i, sizeof(int), 1, file);
        if (ehPrimo(i)) {
            countPrimos++;
        }
    }

    // Escreve a contagem de números primos no final do arquivo
    fwrite(&countPrimos, sizeof(int), 1, file);

    // Fecha o arquivo
    fclose(file);

    printf("Arquivo 'numeros.bin' criado com sucesso!\n");
    printf("Número de primos de 1 até %d: %d\n", n, countPrimos);

    return 0;
}
