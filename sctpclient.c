/*
Para compilar:
Precisa da bibliteca libsctp-dev
gcc sctpclient.c -o client -lsctp
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <netinet/sctp.h> // Novo include

#define ECHOMAX 1024

int main(int argc, char *argv[]) {

	/* Checagem de parametros. Aborta se número de parametros está errado ou se porta é um string invalido. */
	if (argc != 3 || atoi(argv[2]) == 0) {
		printf("Parametros:<remote_host> <remote_port> \n");
		exit(1);
	}

	/* Variáveis Locais */
	int rem_sockfd;
	char linha[ECHOMAX];

	/* Construcao da estrutura do endereco local */
	/* Preenchendo a estrutura socket loc_addr (familia, IP, porta) */
	struct sockaddr_in rem_addr;
	rem_addr.sin_family = AF_INET;				   /* familia do protocolo*/
	rem_addr.sin_addr.s_addr = inet_addr(argv[1]); /* endereco IP local */
	rem_addr.sin_port = htons(atoi(argv[2]));	   /* porta local  */
												   // sin_zero = 0, /* por algum motivo pode se botar isso em 0 usando memset() */

	/* Cria o socket para enviar e receber fluxos */
	/* parametros(familia, tipo, protocolo) */
	rem_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (rem_sockfd < 0) {
		perror("Criando stream socket");
		exit(1);
	}

	printf("> Conectando no servidor '%s:%s'\n", argv[1], argv[2]);

   	/* Estabelece uma conexao remota */
	/* parametros(descritor socket, estrutura do endereco remoto, comprimento do endereco) */
	if (connect(rem_sockfd, (struct sockaddr *) &rem_addr, sizeof(rem_addr)) < 0) {
		perror("Conectando stream socket");
		exit(1);
	}

	do  {
		//gets(linha);
		fgets (linha, ECHOMAX, stdin);
    	linha[strcspn(linha, "\n")] = 0;	

		/* stcp_sendmsg parametros(int s, const void *msg, size_t len, const struct sockaddr *to, socklen_t tolen, 
		uint32_t ppid, uint32_t flags, uint16_t stream_no, uint32_t timetolive, uint32_t context)
		s: descritor socket,
		msg: mensagem a enviar,
		len: tamanho da mensagem, 
		to: endereço de destino NULL é qualquer IP conectado ao socket, NULL pois só ha uma conexão,
			caso haja mais de 1 endereço ligado ao socket se deve usar um endereço específico(rem_addr),
		tolen: tamanho endereço de destino,
		ppid: Payload Protocol Identifier, identifica tipo de mensagem, 0 tipo não específicado,
		flag: sinais que pode mandar: 
			-SCTP_EOF(0x0100) inicia Shutdown, 
			-SCTP_ABORT(0x0200) inicia abortamento, 
			-SCTP_UNORDERED(0x0400) manda desordenado,
			-SCTP_ADDR_OVER(0x0800) escolhe IP específico ignorando IP principal,
			-SCTP_SENDALL(0x1000) manda mensagem para todas as associações do socket.
		stream_no: fluxo escolhido para essa mensagem se é zero o protocolo escolhe,
		timetolive: tempo em ms em que a mensagem expira se falhou de enviar,
		context: valor retornado caso haja erro, pode botar uma variavel aqui para checar qual erro encontra) */
		sctp_sendmsg(rem_sockfd, &linha, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0);

		/* sctp_recvmsg parametros(int s, void *msg, size_t len, struct sockaddr *from, socklen_t *fromlen, 
		struct sctp_sndrcvinfo *sinfo, int *msg_flags) 
		s: descritor socket,
		msg: mensagem recebida,
		len: tamanho da mensagem recebida,
		from: endereço do remetente,
		fromlen: tamanho do endereço do remetente,
		sinfo: informação da mensagem na estrutura to tipo sctp_sndrcvinfo
	    	-sinfo_stream: Fluxo de dados em que chegou.
	    	-sinfo_ssn: Numero de sequencia ddo fluxo.
	    	-sinfo_flags: Flags da mensagem.
	    	-sinfo_ppid: Payload Protocol Identifier da mensagem.
	    	-sinfo_context: Usado se a aplicação tem opção do tipo SCTP_CONTEXT, 
	    		ou para definir uma estrutura de data que deseja receber de uma associação.
	    	-sinfo_timetolive: Tempo de vida, Não é usado em sctp_recvmsg
	    	-sinfo_tsn: Número de sequência.
	    	-sinfo_cumtsn: Número cumulativo de sequência.	
	    	-sinfo_assoc_id: Id da associação com o cliente.
	    flags: sinais que pode receber, são os mesmos que os mandados.
     	*/ 
		sctp_recvmsg(rem_sockfd, &linha, sizeof(linha), NULL, 0, 0, 0);
		printf("Recebi %s\n", linha);
	}while(strcmp(linha,"exit"));
	/* fechamento do socket remota */ 	
	close(rem_sockfd);
}
