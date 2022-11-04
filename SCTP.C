#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
//#include <stdbool.h>

#include <netinet/sctp.h> // Novo include

#define ECHOMAX 1024
#define BUFFER_SIZE 400
void *serverSide(void *);
int main(int argc, char *argv[]);
void *clientSide(void *arg);
void MudaConnect();
int verifyBufferFinalSize(char *buffer, int size);
bool receiveFile = false;
char *rede;
char *ip;
struct sockaddr_in rem_addr;
int rem_sockfd;

// struct params
// {
//     int socket;
//     struct sockaddr_in sockaddr;
// };

// struct params* param;
// 	param->socket = rem_sockfd;
// 	param->sockaddr = rem_addr;

int main(int argc, char *argv[])
{

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <Server IP> <Rede>\n", argv[0]);
		exit(1);
	}

	rede = argv[2];

	pthread_t t_server_id;
	pthread_t t_cliente_id;
	ip = argv[1];

	rem_addr.sin_family = AF_INET;				/* familia do protocolo*/
	rem_addr.sin_addr.s_addr = inet_addr(ip); /* endereco IP local */
	rem_addr.sin_port = htons(8000);			/* porta local  */

	/* Cria o socket para enviar e receber fluxos */
	/* parametros(familia, tipo, protocolo) */
	rem_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (rem_sockfd < 0)
	{
		perror("Criando stream socket");
		exit(1);
	}

	pthread_create(&t_server_id, NULL, serverSide, NULL);
	pthread_create(&t_cliente_id, NULL, clientSide, (void *)argv[1]);
	pthread_join(t_server_id, NULL);
	pthread_join(t_cliente_id, NULL);
	close(rem_sockfd);

	printf("fechando programa");

	return 0;
}

void MudaConnect(char *ipaddr)
{
	
		/* parametros(descritor socket, estrutura do endereco remoto, comprimento do endereco) */
	rem_addr.sin_addr.s_addr = inet_addr(ipaddr); /* endereco IP local */
	if (connect(rem_sockfd, (struct sockaddr *)&rem_addr, sizeof(rem_addr)) < 0)
	{
		perror("Conectando stream socket");
		exit(1);
	}
}

void *clientSide(void *arg)
{
	/* Variáveis Locais */
	char linha[ECHOMAX];
	char *argv = (char *)arg;
	sleep(2);

	/* Construcao da estrutura do endereco local */
	/* Preenchendo a estrutura socket loc_addr (familia, IP, porta) */
	
	

	printf("> Conectando no servidor '%s:%d'\n", argv, 8000);

	/* Estabelece uma conexao remota */
	/* parametros(descritor socket, estrutura do endereco remoto, comprimento do endereco) */
	if (connect(rem_sockfd, (struct sockaddr *)&rem_addr, sizeof(rem_addr)) < 0)
	{
		perror("Conectando stream socket");
		exit(1);
	}

	do
	{
		if(receiveFile)
			continue;
		// gets(linha);

		if(rem_addr.sin_addr.s_addr != inet_addr(ip))
			MudaConnect(ip);

		fgets(linha, ECHOMAX, stdin);
		linha[strcspn(linha, "\n")] = 0;

		if (!strcmp(linha, "change"))
		{
			fprintf(stderr, "digite o novo IP\n");
			fgets(linha, ECHOMAX, stdin);
			linha[strcspn(linha, "\n")] = 0;
			int result = inet_pton(AF_INET, linha, &(rem_addr.sin_addr));
			if (result != 1)
			{
				fprintf(stderr, "Erro, Digite um IP valido\n");
				continue;
			}

			ip = linha;
			if(rem_addr.sin_addr.s_addr != inet_addr(ip))
				MudaConnect(ip);
			continue;
		}

		if (!strcmp(linha, "send"))
		{
			fprintf(stderr, "preparando para receber arquivo\n");
			receiveFile = true;
		}

		sctp_sendmsg(rem_sockfd, &linha, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0);

		sctp_recvmsg(rem_sockfd, &linha, sizeof(linha), NULL, 0, 0, 0);
		printf("Recebi %s\n", linha);
	} while (strcmp(linha, "exit"));
	/* fechamento do socket remota */

	return 0;
}

void *serverSide(void *unused)
{
	int loc_sockfd, loc_newsockfd, tamanho;

	char linha[ECHOMAX];

	/* Construcao da estrutura do endereco local */
	/* Preenchendo a estrutura socket loc_addr (familia, IP, porta) */
	struct sockaddr_in loc_addr;
	loc_addr.sin_family = AF_INET;		   /* familia do protocolo */
	loc_addr.sin_addr.s_addr = INADDR_ANY; /* endereco IP local */
	loc_addr.sin_port = htons(8000);	   /* porta local */

	/* Novas Estrutura: sctp_initmsg, comtém informações para a inicialização de associação*/
	struct sctp_initmsg initmsg;
	initmsg.sinit_num_ostreams = 5;	 /* Número de streams que se deseja mandar. */
	initmsg.sinit_max_instreams = 5; /* Número máximo de streams se deseja receber. */
	initmsg.sinit_max_attempts = 4;	 /* Número de tentativas até remandar INIT. */
									 /*.sinit_max_init_timeo = 60000, Tempo máximo em milissegundos para mandar INIT antes de abortar. Default 60 segundos.*/

	/* Usado para definir quais eventos se deseja verificar. Se recebe usando a estrutura sctp_sndrcvinfo.
	Não foi usado pois acho que é complicado.
	struct sctp_event_subscribe eventos = {};
	*/

	/* Cria o socket para enviar e receber datagramas */
	/* parametros(familia, tipo, protocolo) */
	loc_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP); // Mudança protocolo '0' => 'IPPROTO_SCTP'

	if (loc_sockfd < 0)
	{
		perror("Criando stream socket");
		exit(1);
	}

	/* Bind para o endereco local*/
	/* parametros(descritor socket, estrutura do endereco local, comprimento do endereco) */
	if (bind(loc_sockfd, (struct sockaddr *)&loc_addr, sizeof(struct sockaddr)) < 0)
	{
		perror("Ligando stream socket");
		exit(1);
	}

	/* SCTP necessita usar setsockopt, */
	/* parametros(descritor socket, protocolo, tipo de opção(INIT, EVENTS, etc.), opções, tamanho das opções) */
	if (setsockopt(loc_sockfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg)) < 0)
	{
		perror("setsockopt(initmsg)");
		exit(1);
	}

	/* parametros(descritor socket,	numeros de conexoes em espera sem serem aceites pelo accept)*/
	listen(loc_sockfd, initmsg.sinit_max_instreams); // Mudado para initmsg.sinit_max_instreams.
	printf("> aguardando conexao\n");

	/* Accept permite aceitar um pedido de conexao, devolve um novo "socket" ja ligado ao emissor do pedido e o "socket" original*/
	/* parametros(descritor socket, estrutura do endereco local, comprimento do endereco)*/
	tamanho = sizeof(struct sockaddr_in);
	loc_newsockfd = accept(loc_sockfd, (struct sockaddr *)&loc_addr, (socklen_t *)&tamanho);

	bool filho_iniciado = false;
	do
	{

		if (receiveFile)
		{

			fprintf(stderr, "preparado para receber\n");

			char buffer[BUFFER_SIZE]; 
			bzero(buffer, BUFFER_SIZE);
			FILE *file = fopen("wireSCTP.pcap", "w");
			while (true)
			{
				sctp_recvmsg(loc_newsockfd, &buffer, sizeof(buffer), NULL, 0, 0, 0);
				if (!strcmp(buffer, "END"))
				{
					fprintf(stderr, "Finalizando arquivo\n");
					sctp_sendmsg(loc_newsockfd, &buffer, sizeof(buffer), NULL, 0, 0, 0, 0, 0, 0);
					break;
				}

				int size = verifyBufferFinalSize(buffer, BUFFER_SIZE);
				fprintf(stderr, "recebendo %i bytes\n", size);
				fwrite(buffer, 1, size, file);
				char mensagem[] = "sucesso";
				sctp_sendmsg(loc_newsockfd, &mensagem, sizeof(mensagem), NULL, 0, 0, 0, 0, 0, 0);
			}

			fclose(file);
			fprintf(stderr, "arquivo salvo\n");
			receiveFile = false;
		}

		sctp_recvmsg(loc_newsockfd, &linha, sizeof(linha), NULL, 0, 0, 0);
		printf("Recebi %s\n", linha);

		if (!strcmp(linha, "start"))
		{

			if (filho_iniciado)
			{
				fprintf(stderr, "processo já iniciado\n");
				strcpy(linha, "processo já Iniciado");
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
				const char *arg4 = "tcpDump.pcap";
				int response = execlp(program, program, arg, arg2, arg3, arg4, (char *)NULL);
			}
			else
			{
				filho_iniciado = true;
			}
		}
		else if (!strcmp(linha, "close"))
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
		else if (!strcmp(linha, "send"))
		{
			FILE *file = fopen("tcpDump.pcap", "r");
 
			if (file == NULL)
			{
				perror("File");
				break;
			}

			int size;
			char net_buf[BUFFER_SIZE];
			sctp_sendmsg(loc_newsockfd, &linha, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0);
			if(rem_addr.sin_addr.s_addr != inet_addr(ip))
				MudaConnect(ip);
			strcpy(linha, "enviando arquivo"); 
			sctp_sendmsg(rem_sockfd, &linha, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0);
			sctp_recvmsg(rem_sockfd, &linha, sizeof(linha), NULL, 0, 0, 0);

			while ((size = fread(net_buf, 1, BUFFER_SIZE, file)) > 0)
			{
				fprintf(stderr, "enviando %d bytes\n", size);
				if (size < BUFFER_SIZE)
				{
					net_buf[size] = EOF; 
					size++;
				}

				sctp_sendmsg(rem_sockfd, &net_buf, size, NULL, 0, 0, 0, 0, 0, 0);

				sctp_recvmsg(rem_sockfd, &net_buf, size, NULL, 0, 0, 0);
				printf("Recebido %s\n", linha);
				bzero(net_buf, BUFFER_SIZE);
			}

			char mensagem[] = "END";
			sctp_sendmsg(rem_sockfd, &mensagem, sizeof(mensagem), NULL, 0, 0, 0, 0, 0, 0);
			sctp_recvmsg(rem_sockfd, &mensagem, sizeof(mensagem), NULL, 0, 0, 0);
			fclose(file);
			fprintf(stderr, "Arquivo enviado\n");
			printf("Recebido %s\n", mensagem);

			continue;
		}

		sctp_sendmsg(loc_newsockfd, &linha, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0);
		printf("Renvia %s\n", linha);
	} while (strcmp(linha, "exit"));
	/* fechamento do socket local */
	close(loc_sockfd);
	close(loc_newsockfd);

	return 0;
}


int verifyBufferFinalSize(char *buffer, int size)
{
	fprintf(stderr, "Pesquisando tamanho\n");
	//char *comp = "*";
	for (int i = 0; i < size; i++)
	{
		//char *ptr_somechar = &buffer[i];
		 if (buffer[i] == EOF)
            return i;
	}

	return size;
}