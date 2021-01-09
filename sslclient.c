#include "common.h"

#define MAXBUF          1024
#define NAMESIZE        10
#define CERTFILE "client.pem"

char name[NAMESIZE];

SSL_CTX* setup_client_ctx(void)
{
	SSL_CTX* ctx;

	ctx = SSL_CTX_new(SSLv23_method());
	if (SSL_CTX_use_certificate_chain_file(ctx, CERTFILE) != 1)
		int_error("Error loading certificate from file");
	if (SSL_CTX_use_PrivateKey_file(ctx, CERTFILE, SSL_FILETYPE_PEM) != 1)
		int_error("Error loading private key from file");
	return ctx;
}

char* send_formating(char* name, char* msg)
{
	char* formating = NULL;
	if (formating != NULL)
		free(formating);
	formating = malloc((sizeof(char)) * (MAXBUF + NAMESIZE + 5));
	strcat(formating, "[");
	strcat(formating, name);
	strcat(formating, "] : ");
	strcat(formating, msg);
	return formating;
}

void* send_msg(SSL* ssl)
{
	char msg[MAXBUF + NAMESIZE + 5];
	system("clear");

	welcome_msg();
	usage_msg();
	echo_login_msg(ssl, msg);

	while (1)
	{
		fgets(msg, MAXBUF + NAMESIZE + 5, stdin);
		msg[strlen(msg) - 1] = 0;
		if (strncmp(msg, "!bye", 5) == 0)
		{
			echo_logout_msg(ssl);
			exit(0);
		}
		else if (strncmp(msg, "!clr", 5) == 0)
		{
			system("clear");
		}
		else if (strncmp(msg, "!help", 6) == 0)
		{
			usage_msg();
		}
		else
		{
			char* send_line = malloc((sizeof(char)) * (MAXBUF + NAMESIZE + 5));  //[name] : msg
			send_line = send_formating(name, msg);
			SSL_write(ssl, send_line, strlen(send_line));
			free(send_line);
		}
	}
	SSL_free(ssl);
	return ssl;
}

//utils
void echo_login_msg(SSL* ssl, char* msg) {
	strcat(msg, "<");
	strcat(msg, name);
	strcat(msg, " join the chatting>");
	SSL_write(ssl, msg, strlen(msg));
}

void echo_logout_msg(SSL* ssl) {
	char msg[MAXBUF + NAMESIZE + 5];
	strcat(msg, "<");
	strcat(msg, name);
	strcat(msg, " disconnect the chatting>");
	SSL_write(ssl, msg, strlen(msg));
}

void welcome_msg() {
	printf("***********************************\n");
	printf("welcome to multi chatting program\n");
	printf("***********************************\n");
}

void usage_msg() {
	printf("***********************************\n");
	printf("1. send message: type the word and press enter\n");
	printf("2. clear page: type !bye to disconnect\n");
	printf("3. exit: type !bye to disconnect\n");
	printf("4. help: type !help to see help(this page)\n");
	printf("***********************************\n");
}
//

void* read_msg(SSL *ssl)
{
	char* line = NULL;
	int read;
	while (1)
	{
		line = malloc(sizeof(char) * (MAXBUF + NAMESIZE + 5));
		read = SSL_read(ssl, line, MAXBUF + NAMESIZE + 5);
		if (read <= 0)
			break;
		printf("%s\n", line);
		line = NULL;
	}
	SSL_free(ssl);
	return ssl;
}

int main()
{
	BIO* conn;
	SSL* ssl;
	SSL_CTX* ctx;

	pthread_t send_thread;
	pthread_t read_thread;
	int send_err;
	int read_err;
	char msg_send[MAXBUF];
	char msg_read[MAXBUF];
	char buffer[MAXBUF];

	init_OpenSSL();
	seed_prng();

	ctx = setup_client_ctx();

	conn = BIO_new_connect(SERVER ":" PORT);
	if (!conn)
		int_error("Error creating connection BIO");
	if (BIO_do_connect(conn) <= 0)
		int_error("Error connecting to remote machine");

	if (!(ssl = SSL_new(ctx)))
		int_error("Error creating an SSL context");
	SSL_set_bio(ssl, conn, conn);
	if (SSL_connect(ssl) <= 0)
		int_error("Error connecting SSL object");

	printf("client start.\n");
	printf("input name : \n");
	fgets(name, NAMESIZE, stdin);
	*(name + (strlen(name) - 1)) = 0;
	//thread run
	send_err = pthread_create(&send_thread, NULL, send_msg, ssl);       //send thread
	read_err = pthread_create(&read_thread, NULL, read_msg, ssl);       //read thread
	pthread_join(send_thread, NULL);
	pthread_join(read_thread, NULL);

	SSL_free(ssl);
	SSL_CTX_free(ctx);
	return 0;
}