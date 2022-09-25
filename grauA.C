#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#define ECHOMAX 255
#define BUFFER_SIZE 100
void *serverSide(void *client_thread);
void *clientSide(void *arg);
int verifyBufferFinalSize(char *buffer, int size);
bool receiveFile = false;
char *rede;
char *ip;

int main(int argc, char *argv[])
{

    /* Consistencia */
    /* Consistencia */
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <Server IP> <Rede>\n", argv[0]);
        exit(1);
    }

    rede = argv[2];

    pthread_t t_server_id;
    pthread_t t_cliente_id;
    ip = argv[1];

    pthread_create(&t_server_id, NULL, serverSide, (void *)t_cliente_id);
    pthread_create(&t_cliente_id, NULL, clientSide, (void *)argv[1]);
    pthread_join(t_server_id, NULL);
    pthread_join(t_cliente_id, NULL);
    printf("fechando programa");

    return 0;
}

void *clientSide(void *arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    int sock;
    char *argv = (char *)arg;
    /* Estrutura: familia + endereco IP + porta */
    struct sockaddr_in client;
    // struct sockaddr_in client, client;

    /* Criando Socket */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("Socket Falhou!!!\n");

    int sock_size = sizeof(client);
    char linha[ECHOMAX];
    char aux_exit[ECHOMAX];

    /* Construindo a estrutura de endereco local ----> Se não criar o SO criará
     A funcao bzero eh usada para colocar zeros na estrutura client */
    // bzero((char *)&client, sock_size);
    // client.sin_family = AF_INET;
    // client.sin_addr.s_addr = htonl(INADDR_ANY); /* endereco IP local */
    // client.sin_port = htons(2204); /* porta local (0=auto assign) */
    // bind(sock,(struct sockaddr *)&client, sock_size);

    /* Construindo a estrutura de endereco do destino
     A funcao bzero eh usada para colocar zeros na estrutura client */
    bzero((char *)&client, sock_size);
    client.sin_family = AF_INET;
    /* endereco IP de destino */
    client.sin_addr.s_addr = inet_addr(argv); /* host local */
    client.sin_port = htons(6000);            /* porta do servidor */

    do
    {
        // gets(linha);
        fgets(linha, ECHOMAX, stdin);
        linha[strcspn(linha, "\n")] = 0;

        /* Envia mensagem para o endereco remoto
         parametros(descritor socket, dados, tamanho dos dados, flag, estrutura do socket remoto, tamanho da estrutura) */
        strcpy(aux_exit, linha);
        if (!strcmp(aux_exit, "send"))
        {
            fprintf(stderr, "preparando para receber arquivo\n");
            receiveFile = true;
        }

        sendto(sock, linha, ECHOMAX, 0, (struct sockaddr *)&client, sock_size);
    } while (strcmp(aux_exit, "exit"));
    close(sock);

    return 0;
}

void *serverSide(void *client_thread)
{
    int sock, sendsock;
    /* Estrutura: familia + endereco IP + porta */
    struct sockaddr_in server, client, resend;
    int sock_size = sizeof(server);

    /* Cria o socket para enviar e receber datagramas
       parametros(familia, tipo, protocolo) */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("ERRO na Criacao do Socket!\n");
    else
        printf("Esperando Mensagens...\n");

    char input[ECHOMAX];

    /* Construcao da estrutura do endereco local
     Preenchendo a estrutura socket server (familia, IP, porta)
     A funcao bzero eh usada para colocar zeros na estrutura server */
    bzero((char *)&server, sock_size);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY); /* endereco IP local */
    server.sin_port = htons(6000);              /* porta local  */

    bzero((char *)&resend, sock_size);
    resend.sin_family = AF_INET;
    /* endereco IP de destino */
    resend.sin_addr.s_addr = inet_addr(ip); /* host local */
    resend.sin_port = htons(6000);          /* porta do servidor */
    fprintf(stderr, "Esperando entrada\n");
   

    bool filho_iniciado = false;
    if (-1 != bind(sock, (struct sockaddr *)&server, sock_size))
    {
        do
        {
            if (receiveFile)
            {

                char buffer[BUFFER_SIZE];
                bzero(buffer, BUFFER_SIZE);
                FILE *file = fopen("wire.pcap", "w");
                while (recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client, (socklen_t *)&sock_size))
                {
                    if (!strcmp(buffer, "END"))
                    {
                        break;
                    }

                    int size = verifyBufferFinalSize(buffer, BUFFER_SIZE);
                    fprintf(stderr, "recebendo %i bytes\n", size);
                    fwrite(buffer, 1, size, file);
                }

                fclose(file);
                fprintf(stderr, "arquivo salvo\n");
                receiveFile = false;
            }

            recvfrom(sock, input, ECHOMAX, 0, (struct sockaddr *)&client, (socklen_t *)&sock_size);
            input[strcspn(input, "\n")] = 0;
            fprintf(stderr, "%s\n", input);

            if (!strcmp(input, "start"))
            {

                if (filho_iniciado)
                {
                    fprintf(stderr, "processo já iniciado\n");
                    char resposta[] = "processo já Iniciado";
                    sendto(sock, resposta, sizeof(resposta), 0, (struct sockaddr *)&resend, sock_size);
                    continue;
                }

                /* Recebe mensagem do endereco remoto
                 parametros(descritor socket, dados, tamanho dos dados, flag, estrutura do socket, endereco do tamanho da estrutura) */

                // cria um precesso filho para não travar o pai que vai fechar o filho
                pid_t child_pid = fork();
                if (child_pid == -1)
                    perror("fork");
                else if (child_pid == 0)
                {
                    // inicia o script
                    const char *program = "tcpdump";
                    const char *arg = "-i";
                    const char *arg2 = rede;
                    const char *arg3 = "-w";
                    const char *arg4 = "teste.pcap";
                    int response = execlp(program, program, arg, arg2, arg3, arg4, (char *)NULL);
                }
                else
                {
                    filho_iniciado = true;
                }
            }
            else if (!strcmp(input, "close"))
            {
                // finaliza o script
                pid_t child_pid = fork();
                if (child_pid == -1)
                    perror("fork");
                else if (child_pid == 0)
                {
                    const char *program = "pkill";
                    const char *arg = "tcpdump";
                    execlp(program, program, arg, (char *)NULL);
                }
                else
                    filho_iniciado = false;
            }
            else if (!strcmp(input, "send"))
            {
                FILE *file = fopen("teste.pcap", "r");

                if (file == NULL)
                {
                    perror("File");
                    break;
                }

                char net_buf[BUFFER_SIZE];
                int size;
                char mensagem[BUFFER_SIZE] = "enviando arquivo";
                sendto(sock, mensagem, sizeof(mensagem), 0, (struct sockaddr *)&resend, sock_size);
                while ((size = fread(net_buf, 1, BUFFER_SIZE, file)) > 0)
                {
                    
                    fprintf(stderr, "enviando %d bytes\n", size);
                    if (size < BUFFER_SIZE){
                        net_buf[size] = EOF;
                        size++;
                    }

                    sendto(sock, net_buf, size, 0, (struct sockaddr *)&resend, sock_size);
                    bzero(net_buf, BUFFER_SIZE);
                }

                strcpy(mensagem, "END");
                sendto(sock, mensagem, sizeof(mensagem), 0, (struct sockaddr *)&resend, sock_size);
                fclose(file);
                fprintf(stderr, "Arquivo enviado\n");

            }

        } while (strcmp(input, "exit"));
        close(sock);
    }

    return 0;
}

int verifyBufferFinalSize(char *buffer, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (buffer[i] == EOF)
            return i;
    }

    return size;
}