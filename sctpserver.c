/*
Para compilar:
Precisa da bibliteca libsctp-dev
gcc sctpserver.c -o server -lsctp
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/sctp.h> // Novos includes
#include <sys/types.h>

#define ECHOMAX 1024

int main(int argc, char *argv[]) {

	/* Checagem de parametros. */
	if (argc != 2) {
		printf("Parametros: <local_port> \n");
		exit(1);
	}

	/* Variaveis Locais */
	int loc_sockfd, loc_newsockfd, tamanho;
	
	char linha[ECHOMAX];

	/* Construcao da estrutura do endereco local */
	/* Preenchendo a estrutura socket loc_addr (familia, IP, porta) */
	struct sockaddr_in loc_addr;
	loc_addr.sin_family = AF_INET;			  /* familia do protocolo */
	loc_addr.sin_addr.s_addr = INADDR_ANY;	  /* endereco IP local */
	loc_addr.sin_port = htons(atoi(argv[1])); /* porta local */
											  //.sin_zero = 0, /* por algum motivo pode se botar isso em 0 usando memset() */

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
	
	if (loc_sockfd < 0) {
		perror("Criando stream socket");
		exit(1);
	}

   	/* Bind para o endereco local*/
	/* parametros(descritor socket, estrutura do endereco local, comprimento do endereco) */
	if (bind(loc_sockfd, (struct sockaddr *) &loc_addr, sizeof(struct sockaddr)) < 0) {
		perror("Ligando stream socket");
		exit(1);
	}
	
	/* SCTP necessita usar setsockopt, */
	/* parametros(descritor socket, protocolo, tipo de opção(INIT, EVENTS, etc.), opções, tamanho das opções) */
  	if (setsockopt (loc_sockfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof (initmsg)) < 0){
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

	do  {

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
		sctp_recvmsg(loc_newsockfd, &linha, sizeof(linha), NULL, 0, 0, 0);
		printf("Recebi %s\n", linha);

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
		sctp_sendmsg(loc_newsockfd, &linha, sizeof(linha), NULL, 0, 0, 0, 0, 0, 0);
		printf("Renvia %s\n", linha);

	}while(strcmp(linha,"exit"));
	/* fechamento do socket local */ 
	close(loc_sockfd);
	close(loc_newsockfd);
}


