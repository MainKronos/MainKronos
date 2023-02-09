#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include <cjson/cJSON.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#define BUFFER_LEN 4096 // Dimenzione dei buffer

int base64_encode(char input[], int length) {
	BIO *b64, *bmem;
	BUF_MEM *bptr;

	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, input, strlen(input));
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

	if(bptr->length >= length) return -1;

	memcpy(input, bptr->data, bptr->length);
	input[bptr->length] = 0;

	BIO_free_all(b64);

	return 0;
}


/**
 * Genera una richiesta HTTPS
 * @param url Indirizzo url
 * @param headers Headers richiesta HTTP
 * @param auth "username:password"
 * @return Il corpo della risposta HTTP
*/
char* https_request(char url[], char headers[], char auth[]){
	/* --- Variabili --------------------------------------------------------------- */
	struct sockaddr_in sv_addr; // Indirizzo server
	struct hostent *host; // Struttura host server
	char hostname[BUFFER_LEN]; // Nome host server
	char port[8]; // Porta del server
	char protocol[8]; // Protocollo utilizzato
	char* auth_b64; // Credenziali in base64
	int sd; // Descrittore Socket
	struct ssl_st *conn; // Connessione SSL
	struct ssl_ctx_st *ssl_ctx; // Contesto SSL
	char packet[BUFFER_LEN]; // Buffer per la richiesta http
	char buffer[BUFFER_LEN]; // Buffer per la risposta http
	char* body; // Buffer per il body della risposta
	int size; // Grandezza del buffer body
	int ret; // Variabile di ritorno
	char* tmp; // Variabile temporanea per la minipolazione delle stringhe
	/* ----------------------------------------------------------------------------- */

	/* --- Parsing URL ------------------------------------------------------------- */
	// Rimuovo il carattere finale '/' se ci fosse
	if(url[strlen(url)] == '/') 
		url[strlen(url)] = '\0';
	
	// Copio il protocollo
	ret = strchr(url, ':')-url;
	strncpy(protocol, url, ret);
	protocol[ret] = '\0';

	// Controllo il protocollo
	assert(0 == strcmp(protocol, "https"));

	// Copio il nome host del server
	strncpy(hostname, strstr(url, "://")+3, sizeof(hostname));

	// Controllo se è definita la porta
	if(tmp = strchr(hostname, ':')) {
		strncpy(port, tmp+1,  sizeof(port));
		*tmp = '\0';
	}else{
		strcpy(port, "443");
	}

	// Controllo se è definita una query
	if(tmp = strchr(hostname, '?'))
		*tmp = '\0';

	// Controllo se ci sono delle pagine
	if(tmp = strchr(hostname, '/'))
		*tmp = '\0';
	
	/* --- Generazione pacchetto HTTP ---------------------------------------------- */
	// Pulizia stringa
	memset(packet, 0, BUFFER_LEN);

	// Inserimento delle informazioni base
	sprintf(packet,
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n",
		url, hostname
	);

	// Aggiungo l'header
	if(headers){
		strcat(packet, headers);
	}else{
		strcat(packet, 
			"User-Agent: curl/7.64.1\r\n"
			"Accept-Language: it,en\r\n"
		);
	}

	// Aggiungo l'autenticazione
	if(auth){
		int auth_b64_len = 4 * ceil((double)strlen(auth) / 3) + 1;
		auth_b64 = malloc(auth_b64_len);
		strcpy(auth_b64, auth);
		base64_encode(auth_b64, auth_b64_len);

		strcat(packet, "Authorization: Basic ");
		strcat(packet, auth_b64);
		strcat(packet, "\r\n");

		free(auth_b64);
	}

	// Aggiungo il fine header
	strcat(packet, "\r\n");

	/* --- Inizializzazione SSL ---------------------------------------------------- */
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	SSL_library_init();

	if(!(ssl_ctx = SSL_CTX_new(SSLv23_client_method()))){
		fprintf(stderr, "ERROR SSL_CTX_new\n");
		exit(EXIT_FAILURE);
	}
	SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2);

	/* --- Connessione ------------------------------------------------------------- */

	// Creazione di una connessione SSL
	if(!(conn = SSL_new(ssl_ctx))){
		fprintf(stderr, "ERROR SSL_new\n");
		exit(EXIT_FAILURE);
	}

	// risoluzione hostname
	if (!(host = gethostbyname(hostname))) {
		perror("ERROR gethostbyname");
		exit(EXIT_FAILURE);
	}

	// crerazione socket
	sd = socket(AF_INET, SOCK_STREAM, 0);

	// Creazione indirizzo del server
	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(atoi(port));
	sv_addr.sin_addr.s_addr = *(long*)host->h_addr;

	// connessione al server
	if(connect(sd, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) < 0){
		perror("ERROR connect");
		exit(EXIT_FAILURE);
	}

	// Collegamento del socket alla struttura SSL
	if(!SSL_set_fd(conn, sd)){
		fprintf(stderr, "ERROR SSL_set_fd\n");
		exit(EXIT_FAILURE);
	}

	// Connessione al server tramite SSL
	if((ret = SSL_connect(conn)) < 1){
		fprintf(stderr, "ERROR SSL_connect: SSL error #%d\n", SSL_get_error(conn, ret));
		exit(EXIT_FAILURE);
	}
	
	/* --- Invio richiesta --------------------------------------------------------- */
	if(SSL_write(conn, packet, strlen(packet)) < 0){
		perror("ERROR SSL_write");
		exit(EXIT_FAILURE);
	}

	/* --- Ricezione richiesta ----------------------------------------------------- */
	int step = 0;
	while((ret = SSL_read(conn, buffer, sizeof(buffer)-1)) != 0){
		if(ret < 0){
			perror("ERROR recv");
			exit(EXIT_FAILURE);
		}
		buffer[ret] = '\0';

		if(step == 0){
			if(tmp = strstr(buffer, "Content-Length: ")){
				sscanf(tmp, "Content-Length: %d", &size);
				size++;
				body = malloc(size * sizeof(char));
				memset(body, 0, size * sizeof(char));
				step++;
			}
		}
		if(step == 1){
			if(tmp = strstr(buffer, "\r\n\r\n")){
				strcpy(buffer, tmp+4);
				step++;
			}
		}
		if(step == 2){
			strcat(body, buffer);
			
			if(strlen(body) == size-1) break;
		}
	}

	/* --- Chiusura collegamento --------------------------------------------------- */
	SSL_shutdown(conn);
	close(sd);
	SSL_free(conn);
	SSL_CTX_free(ssl_ctx);

	return body;
}

void genActivity(){
	char url[BUFFER_LEN]; // Buffer per contenere l'url
	char* buffer; // Buffer risposta http
	cJSON* root; // Struttura per manipolare il json

	char headers[] = "User-Agent: curl/7.64.1\r\n"
		"Accept-Language: it,en\r\n"
		"Accept: application/vnd.github+json\r\n"
		"X-GitHub-Api-Version: 2022-11-28\r\n";
	
	char auth[1024] = "MainKronos:";
	strcat(auth, getenv("GITHUB_TOKEN") ? getenv("GITHUB_TOKEN") : "");

	// Ricerca degli ultimi 3 reporitory modificati
	strcpy(url, "https://api.github.com/user/repos?visibility=all&affiliation=owner&sort=pushed&direction=desc&per_page=3&page=1");
	buffer = https_request(url, headers, auth);
	root = cJSON_Parse(buffer);

	printf("\n");
	for(cJSON* repo = (root != NULL) ? (root)->child : NULL; repo != NULL; repo = repo->next){
		char* buffer2; // Buffer per la seconda richiesta http
		cJSON* root2; // Struttura per manipolare il json

		printf(
			"### %s [%s](%s)\n"
			"> %s\n",
			cJSON_GetObjectItem(repo, "private")->valueint ? "🔒" : "🔓",
			cJSON_GetObjectItem(repo, "name")->valuestring,
			cJSON_GetObjectItem(repo, "html_url")->valuestring,
			cJSON_GetObjectItem(repo, "description")->valuestring
		);
		
		// Ricerca degli ultimi 3 commit fatti
		sprintf(
			url, "https://api.github.com/repos/MainKronos/%s/commits?author=MainKronos&per_page=3&page=1",
			cJSON_GetObjectItem(repo, "name")->valuestring
		);
		buffer2 = https_request(url, headers, auth);
		root2 = cJSON_Parse(buffer2);

		for(cJSON* commit = (root2 != NULL) ? (root2)->child : NULL; commit != NULL; commit = commit->next){
			char* tmp;
			char* message = cJSON_GetObjectItem(cJSON_GetObjectItemCaseSensitive(commit, "commit"), "message")->valuestring;

			// Rimuovo il testo dopo '\n' perchè altrimenti troppo lungo
			if(tmp = strchr(message, '\n')) tmp[0] = '\0';
			
			printf(
				"- 📌 [%s](%s)\n",
				message,
				cJSON_GetObjectItem(commit, "html_url")->valuestring
			);
		}

		cJSON_Delete(root2);
		free(buffer2);
	}
	printf("\n");

	cJSON_Delete(root);
	free(buffer);
}

int main(int argc, char *argv[]){
	
	assert(argc == 2);

	if(strcmp("genActivity", argv[1]) == 0)
		genActivity();
	else
		exit(-1);

	return 0;
}